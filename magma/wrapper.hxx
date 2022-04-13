


#include "../include/cornerstone.hxx"
#include <iostream>
#include <cassert>
#include <map>

using namespace cornerstone;

// extern void cleanup(const std::string& folder);

#ifdef _WIN32
extern int mkdir(const char* path, int mode);
extern int rmdir(const char* path);
#else
#define LOG_INDEX_FILE "/store.idx"
#define LOG_DATA_FILE "/store.dat"
#define LOG_START_INDEX_FILE "/store.sti"
#define LOG_INDEX_FILE_BAK "/store.idx.bak"
#define LOG_DATA_FILE_BAK "/store.dat.bak"
#define LOG_START_INDEX_FILE_BAK "/store.sti.bak"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif


ptr<asio_service> asio_svc_;
std::condition_variable stop_cv1;
std::mutex lock1;

class fs_logger: public logger {
public:
    fs_logger(const std::string& filename): fs_(filename) {}

    __nocopy__(fs_logger);

public:
    virtual void debug(const std::string& log_line) {
        fs_ << "DEBUG" << log_line << std::endl;
        fs_.flush();
    }

    virtual void info(const std::string& log_line) {
        fs_ << "INFO" << log_line << std::endl;
        fs_.flush();
    }

    virtual void warn(const std::string& log_line) {
        fs_ << "WARN" << log_line << std::endl;
        fs_.flush();
    }

    virtual void err(const std::string& log_line) {
        fs_ << "ERROR" << log_line << std::endl;
        fs_.flush();
    }

private:
    std::ofstream fs_;
};

class simple_state_mgr: public state_mgr{
public:
    simple_state_mgr(int id, std::string ip)
        : self_id(id), self_ip(ip) {
        store_path_ = sstrfmt("store%d").fmt(self_id);

        // Initial cluster config: contains only one server (myself).
        my_srv_config_ = cs_new<srv_config>( self_id, self_ip );
        saved_config_ = cs_new<cluster_config>();
        saved_config_->get_servers().push_back(my_srv_config_);
    }

public:
    virtual ptr<cluster_config> load_config() {
        return saved_config_;
        // ptr<cluster_config> conf = cs_new<cluster_config>();
        // for (const auto& srv : cluster_) {
        //     conf->get_servers().push_back(srv);
        // }

        // return conf;
    }

    virtual void save_config(cluster_config& config) {
        ptr<buffer> buf = config.serialize();
        saved_config_ = cluster_config::deserialize(*buf);
    }

    virtual void save_state(const srv_state& state) {
        // ptr<buffer> buf = state.serialize(); TODO
        // saved_state_ = srv_state::deserialize(*buf);
    }

    virtual ptr<srv_state> read_state() {
        // return cs_new<srv_state>();
        return saved_state_;
    }

    virtual ptr<log_store> load_log_store() {
        mkdir(store_path_.c_str(), 0766);
        return cs_new<fs_log_store>(store_path_);
    }

    virtual int32 server_id() {
        return self_id;
    }

    virtual void system_exit(const int exit_code) {
        std::cout << "system exiting with code " << exit_code << std::endl;
    }

private:
    int32 self_id;
    std::string self_ip;
    std::vector<ptr<srv_config>> cluster_;
    std::string store_path_;
    ptr<srv_config> my_srv_config_;
    ptr<cluster_config> saved_config_;
    ptr<srv_state> saved_state_;
};


class localDB {
    public:
        int getValue (char* key) {
            // check if key is present
            if (m.find(key) != m.end())
                return m[key];
            return -1;
        }

        void setValue (std::string key, int32 value) {
            m[key] = value;
            std::cout << "Saved value = " << value << std::endl;
            // save to the cloud too
            // const char* cmd = lstrfmt("curl --location 'https://iotproj.deadcat.xyz' --request POST --header 'Content-Type: text/plain' --data-raw '%d' > /dev/null").fmt(value);
            // std::system(cmd);
        }
    
    private:
        std::map<std::string, int> m;
};


class magma;
void run_raft_instance_with_asio(bool enable_prevote, magma* magmaptr);

// Constructor: ip, id, port
class magma {
    public:
        // magma(const std::vector<ptr<srv_config>>& cluster, const int32 self_id, int32 self_port) {
        magma(const std::string self_ip, const int32 self_id, int self_port) {
            magma::self_id = self_id;
            // magma::_cluster = cluster;
            magma::self_ip = self_ip;
            magma::self_port = self_port;
            asio_svc_ = cs_new<asio_service>();
            ptr<state_mgr> smgr_(cs_new<simple_state_mgr>(self_id, self_ip));
            smgr = smgr_;

            std::thread t1([this]{
                run_raft_instance_with_asio(false, this);
            });
            t1.detach();

            localDB local_db;

            std::cout << "waiting for leader election..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));

            client = asio_svc_->create_client(self_ip);
        };
        ~magma() {
            stop_cv1.notify_all();
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            asio_svc_->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // TODO
            // std::remove(sstrfmt("log%d.log").fmt(self_id));
            // cleanup(sstrfmt("store%d").fmt(self_id));
            // rmdir(sstrfmt("store%d").fmt(self_id));
        };

    public:
        int get(char* key) {
            return local_db.getValue(key);
        };

        void set(char* key, int value) {
            sendMsg(sstrfmt("%s@%d").fmt(key, value));
        };

        void addServer (int id, std::string ip) {
            srv_config srv_conf_to_add( id, ip );
            raft_server_->add_srv(srv_conf_to_add);
        };


        void notifyAll(const char* usrmsg) { sendMsg(usrmsg); };
    
    private:
        void sendMsg(const char* data) {
            ptr<req_msg> msg = cs_new<req_msg>(0, msg_type::client_request, 0, 1, 0, 0, 0);
            bufptr buf = buffer::alloc(100); // TODO check data to be below 100 bytes
            buf->put(data);
            buf->pos(0);
            msg->log_entries().push_back(cs_new<log_entry>(0, std::move(buf)));
            rpc_handler handler = (rpc_handler)([this, data](ptr<resp_msg>& rsp, const ptr<rpc_exception>& err) -> void {
                if (err) {
                    std::cout << "Err: " << err->what() << std::endl;
                }

                if (!rsp->get_accepted()) {
                    // we need to message the leader instead!

                    // std::cout << "dest id: " << rsp->get_dst() << " ip: "; // TODO
                    ptr<srv_config> srv_conf_ = smgr->load_config()->get_server(rsp->get_dst());
                    if (srv_conf_) {
                        // std::cout << srv_conf_->get_endpoint() << std::endl;
                        client = asio_svc_->create_client(srv_conf_->get_endpoint());
                    } else {
                        std::cout << "Server not found." << std::endl;
                        return;
                    }
                    
                    ptr<req_msg> msg = cs_new<req_msg>(0, msg_type::client_request, 0, 1, 0, 0, 0);
                    bufptr buf = buffer::alloc(100);
                    buf->put(data);
                    buf->pos(0);
                    msg->log_entries().push_back(cs_new<log_entry>(0, std::move(buf)));
                    rpc_handler handler = (rpc_handler)([](ptr<resp_msg>& rsp1, const ptr<rpc_exception>& err_) -> void {
                        if (err_ || !rsp1->get_accepted()) {
                            if (err_) {
                                std::cout << "Err: " << err_->what() << std::endl;
                            }
                            std::cout << "Error in sending message!" << std::endl;
                        }
                    });
                    client->send(msg, handler);
                }
            });

            client->send(msg, handler);
        }
    
    private:
        ptr<rpc_client> client;
        // ptr<req_msg> msg;
        // bufptr buf = buffer::alloc(100);
    
    public:
        int32 self_id;
        int self_port;
        std::string self_ip;
        localDB local_db;
        // std::vector<ptr<srv_config>> _cluster;
        ptr<raft_server> raft_server_;
        ptr<state_mgr> smgr;

};


class echo_state_machine : public state_machine {
public:
    echo_state_machine(localDB* local_dbptr) : lock_(), local_dbptr_(local_dbptr) {}
public:
    virtual void commit(const ulong, buffer& data, const uptr<log_entry_cookie>&) {
        auto_lock(lock_);
        // std::cout << "commit message:" <<  reinterpret_cast<const char*>(data.data()) << std::endl;
        const char* data_ = reinterpret_cast<const char*>(data.data());
        const std::string d = std::string(reinterpret_cast<const char*>(data.data()));
        if (d.length() <= 0) return;
        const int indx = d.find_last_of('@');
        local_dbptr_->setValue(d.substr(0, indx), std::stoi(d.substr(indx+1)));
    }

    virtual void pre_commit(const ulong, buffer& data, const uptr<log_entry_cookie>&) {
        auto_lock(lock_);
        // std::cout << "pre-commit: " << reinterpret_cast<const char*>(data.data()) << std::endl;
    }

    virtual void rollback(const ulong, buffer& data, const uptr<log_entry_cookie>&) {
        auto_lock(lock_);
        // std::cout << "rollback: " << reinterpret_cast<const char*>(data.data()) << std::endl;
    }

    virtual void save_snapshot_data(snapshot&, const ulong, buffer&) {}
    virtual bool apply_snapshot(snapshot&) {
        return false;
    }

    virtual int read_snapshot_data(snapshot& , const ulong, buffer&) {
        return 0;
    }

    virtual ulong last_commit_index() {
        return 0;
    }

    virtual ptr<snapshot> last_snapshot() {
        return ptr<snapshot>();
    }

    virtual void create_snapshot(snapshot&, async_result<bool>::handler_type&) {}
private:
    std::mutex lock_;
    // magma* db_conn_;
    localDB* local_dbptr_;
};


class test_event_listener: public raft_event_listener {
public:
    test_event_listener() : raft_event_listener() {}

public:
    virtual void on_event(raft_event event) override {
        // TODO save our state; to show incase we are leader
        switch (event)
        {
        case raft_event::become_follower:
            std::cout << "becomes a follower" << std::endl;
            break;
        case raft_event::become_leader:
            std::cout << "becomes a leader" << std::endl;
            break;
        case raft_event::logs_catch_up:
            std::cout << "catch up all logs" << std::endl;
            break;
        }
    }
};


void run_raft_instance_with_asio(bool enable_prevote, magma* magmaptr ) {
    ptr<logger> l(cs_new<fs_logger>(sstrfmt("log%d.log").fmt(magmaptr->self_id)));
    ptr<rpc_listener> listener(asio_svc_->create_rpc_listener(magmaptr->self_port, l));
    ptr<state_machine> smachine(cs_new<echo_state_machine>(&magmaptr->local_db));
    
    raft_params* params(new raft_params());
    (*params).with_election_timeout_lower(200)
        .with_election_timeout_upper(400)
        .with_hb_interval(100)
        .with_max_append_size(100)
        .with_rpc_failure_backoff(50)
        .with_prevote_enabled(enable_prevote);
    ptr<delayed_task_scheduler> scheduler = asio_svc_;
    ptr<rpc_client_factory> rpc_cli_factory = asio_svc_;
    context* ctx(new context(magmaptr->smgr, smachine, listener, l, rpc_cli_factory, scheduler, cs_new<test_event_listener>(), params));
    ptr<raft_server> server(cs_new<raft_server>(ctx));
    listener->listen(server);

    magmaptr->raft_server_ = server;

    {
        std::unique_lock<std::mutex> ulock(lock1);
        stop_cv1.wait(ulock);
        listener->stop();
    }
}
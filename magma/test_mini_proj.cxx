
#include "wrapper.hxx"

using namespace cornerstone;





void test_mini_proj(int argc, char** argv) {
    // auto cluster = std::vector<ptr<srv_config>>({
    //     cs_new<srv_config>(1, "tcp://127.0.0.1:9000"),
    //     // cs_new<srv_config>(2, "tcp://192.168.33.252:9000"),
    //     // cs_new<srv_config>(3, "tcp://:9000")
    // });
    std::string default_ip = "tcp://127.0.0.1:9000";
    int default_port = 9000;
    int nservs = 1;

    if (argc == 4) {
        default_ip = argv[1];
        default_port = std::atoi(argv[2]);
        nservs = std::atoi(argv[3]);
    }

    magma sv1(default_ip, nservs, default_port);
    nservs++;
    // magma sv2(cluster, 2);
    // magma sv3(cluster, 3);

    // for (int i = 0; i < 6; i++){
    //     std::this_thread::sleep_for(std::chrono::seconds(15));
    //     sv1.set("value", i);
    //     std::cout << "Current Value in Node 1: " << sv1.get("value") << std::endl;
    // }
    // std::string l;
    // std::cout << "IP: ";
    // std::cin >> l;

    // if (l != "\n") {
    //     sv1.addServer(k, l);

    //     int i = 0;
    sv1.addServer(1, "tcp://127.0.0.1:9000");
    std::string cmd;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, cmd);

        if (cmd.substr(0, 3) == "add") {
            sv1.addServer(nservs, cmd.substr(4));
            nservs++;
        } else if (cmd.substr(0, 3) == "set") {
            sv1.set("value", std::atoi(cmd.substr(4).c_str()));
        } else if (cmd.substr(0, 3) == "abt") {
            std::cout << (sv1.raft_server_->is_leader() ? "We are leader" : "We are follower") << std::endl;
        } else if (cmd.substr(0, 3) == "val") {
            std::cout << "Value: " << sv1.get("value") << std::endl;
        }
        // sv1.set("value", i);
        // std::cout << "Current Value: " << sv1.get("value") << std::endl;
        // i = (i + 1) % 10;
        // std::cout << sv1.raft_server_->is_leader() << std::endl;
        // std::this_thread::sleep_for(std::chrono::seconds(1));
    }
// } else {
    //     while(true);
    // }
}
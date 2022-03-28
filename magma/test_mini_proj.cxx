
#include "wrapper.hxx"

using namespace cornerstone;

void test_mini_proj() {
    auto cluster = std::vector<ptr<srv_config>>({
        cs_new<srv_config>(1, "tcp://127.0.0.1:9001"),
        cs_new<srv_config>(2, "tcp://127.0.0.1:9002"),
        cs_new<srv_config>(3, "tcp://127.0.0.1:9003")
    });
    magma sv1(cluster, 1);
    magma sv2(cluster, 2);
    magma sv3(cluster, 3);

    // const char* msg1 = "I am King!";
    // const char* msg2 = "I am Second!";
    // const char* msg3 = "I am Ultron!";

    for (int i = 0; i < 10; i++){
        std::this_thread::sleep_for(std::chrono::seconds(5));
        // sv1.notifyAll(msg1);
        // sv2.notifyAll(msg2);
        // sv3.notifyAll(msg3);
        sv2.set("value", i);
        std::cout << "Current Value in Node 1: " << sv1.get("value") << std::endl;
    }
}
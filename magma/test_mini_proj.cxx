
#include "wrapper.hxx"

using namespace cornerstone;

void test_mini_proj() {
    auto cluster = std::vector<ptr<srv_config>>({
        cs_new<srv_config>(1, "tcp://127.0.0.1:9000"),
        cs_new<srv_config>(2, "tcp://127.0.0.1:9000"),
        cs_new<srv_config>(3, "tcp://127.0.0.1:9000")
    });
    magma sv1(cluster, 1);
    // magma sv2(cluster, 2);
    // magma sv3(cluster, 3);

    // for (int i = 0; i < 6; i++){
    //     std::this_thread::sleep_for(std::chrono::seconds(15));
    //     sv1.set("value", i);
    //     std::cout << "Current Value in Node 1: " << sv1.get("value") << std::endl;
    // }
    int i = 0;
    while (true) {
        sv1.set("value", i);
        std::cout << "Current Value: " << sv1.get("value") << std::endl;
        i = (i + 1) % 10;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
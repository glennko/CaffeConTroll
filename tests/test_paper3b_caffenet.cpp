#include "../src/DeepNet.h"
#include "gtest/gtest.h"
#include "../src/Kernel.h"
#include "../src/LogicalCube.h"
#include "../src/Layer.h"
#include "../src/util.h"
#include "../src/Connector.h"
#include "../src/bridges/DropoutBridge.h"
#include "test_types.h"


TEST(CaffenetPaper3bTrainTest, RunTest) {

  char const * a = "deepnet";
  char const * b = "train";
  //char const * c = "tests/imagenet_train/solver/imagenet_snapshot_solver_6_iters_2_batch.prototxt";
  char const * c = "tests/imagenet_train/solver/caffenet_solver_1000.prototxt";
  char const * argv[3];
  argv[0] = a;
  argv[1] = b;
  argv[2] = c;
  std::string data_binary = "tests/imgnet_toprocess.bin";
  std::string model_file = "tests/model.bin";

  DeepNet::load_and_train_network(argv[2], data_binary, "NA", model_file, "");
  //float acc = DeepNet::load_and_test_network(argv[2], data_binary, model_file);

  // std::fstream expected_accuracy("tests/output/accuracy_train.txt", std::ios_base::in);
  // double output;
  // if (expected_accuracy.is_open()) {
  //   expected_accuracy >> output;
  //   EXPECT_NEAR(acc, output, 0.02);
  // }else{
  //   FAIL();
  // }
  // expected_accuracy.close();

}

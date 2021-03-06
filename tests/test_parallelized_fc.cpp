// This test creates a conv bridge and an fc bridge
// When kernel size = fmap size, and p=0, and s= anything,
// fc and conv are same. So, this ensures that the fc bridge
// gives the same output as the conv bridge under those
// circumstances.

#include "../src/Kernel.h"
#include "../src/LogicalCube.h"
#include "../src/Layer.h"
#include "../src/util.h"
#include "../src/Connector.h"
#include "../src/bridges/ConvolutionBridge.h"
#include "../src/bridges/FullyConnectedBridge.h"
#include "../src/bridges/ParallelizedBridge.h"
#include "test_types.h"
#include "gtest/gtest.h"
#include <iostream>
#include <fstream>
#include <assert.h>
#include <cmath>
#include <cstring>

template <typename TypeParam>
class ParallelizedFCBridgeTest : public ::testing::Test {
  public:
    typedef typename TypeParam::T T;
    ParallelizedFCBridgeTest(){
      data1 = new LogicalCube<T, Layout_CRDB>(iR, iC, iD, mB);
      grad1 = new LogicalCube<T, Layout_CRDB>(iR, iC, iD, mB);

      data2 = new LogicalCube<T, Layout_CRDB>(oR, oC, oD, mB);
      grad2 = new LogicalCube<T, Layout_CRDB> (oR, oC, oD, mB);

      layer1 = new Layer<T, Layout_CRDB>(data1, grad1);
      layer2 = new Layer<T, Layout_CRDB>(data2, grad2);

      data1c = new LogicalCube<T, Layout_CRDB>(iR, iC, iD, mB);
      grad1c = new LogicalCube<T, Layout_CRDB>(iR, iC, iD, mB);

      data2c = new LogicalCube<T, Layout_CRDB>(oR, oC, oD, mB);
      grad2c = new LogicalCube<T, Layout_CRDB> (oR, oC, oD, mB);

      layer1c = new Layer<T, Layout_CRDB>(data1c, grad1c);
      layer2c = new Layer<T, Layout_CRDB>(data2c, grad2c);

      cnn::LayerParameter layer_param;
      layer_param.set_gpu_0_batch_proportion(0);
      cnn::ConvolutionParameter * const conv_param = layer_param.mutable_convolution_param();
      conv_param->set_num_output(oD);
      conv_param->set_kernel_size(k);
      conv_param->set_pad(p);
      conv_param->set_stride(s);

      solver_param.set_base_lr(0.01);
      solver_param.set_momentum(0.0);
      solver_param.set_lr_policy("step");
      solver_param.set_stepsize(10000);

      cnn::InnerProductParameter * const inn_param = layer_param.mutable_inner_product_param();
      inn_param->set_num_output(oD);

      ParallelizedConvolutionBridge_ = new ParallelizedBridge<DataType_SFFloat,
              ConvolutionBridge>(layer1, layer2, &layer_param, &solver_param, &pdriver, 1, 1);

      ParallelizedFCBridge_ = new ParallelizedBridge<DataType_SFFloat,
              FullyConnectedBridge>(layer1c, layer2c, &layer_param, &solver_param, &pdriver, 1, 1);

      ParallelizedConvolutionBridge_->needs_to_calc_backward_grad = true;
    }

    virtual ~ParallelizedFCBridgeTest() {
		delete layer1; 
		delete layer2;
		delete layer1c; 
		delete layer2c;
		// grad and data for each later are deleted by the layer destructor
		// Also delete the parallelized bridge
		delete ParallelizedConvolutionBridge_;
		delete ParallelizedFCBridge_;
	}
    ParallelizedBridge<DataType_SFFloat,
              ConvolutionBridge>* ParallelizedConvolutionBridge_;
    ParallelizedBridge<DataType_SFFloat,
              FullyConnectedBridge>* ParallelizedFCBridge_;

    LogicalCube<T, Layout_CRDB>* data1;
    LogicalCube<T, Layout_CRDB>* grad1;

    LogicalCube<T, Layout_CRDB>* data2;
    LogicalCube<T, Layout_CRDB>* grad2;

    Layer<T, Layout_CRDB>* layer1;
    Layer<T, Layout_CRDB>* layer2;

    LogicalCube<T, Layout_CRDB>* data1c;
    LogicalCube<T, Layout_CRDB>* grad1c;

    LogicalCube<T, Layout_CRDB>* data2c;
    LogicalCube<T, Layout_CRDB>* grad2c;

    Layer<T, Layout_CRDB>* layer1c;
    Layer<T, Layout_CRDB>* layer2c;

    cnn::SolverParameter solver_param;

    CPUDriver pdriver;

    static const int mB = 7;
    static const int iD = 12;
    static const int oD = 25;
    static const int iR = 20;
    static const int iC = 20;
    static const int k = 20;
    static const int s = 1;
    static const int p = 0;
    static const int oR = static_cast<int>((static_cast<float>(iR + 2*p - k) / s)) + 1;
    static const int oC = static_cast<int>((static_cast<float>(iC + 2*p - k) / s)) + 1;
};

typedef ::testing::Types<FloatNOFUNC> DataTypes;

TYPED_TEST_CASE(ParallelizedFCBridgeTest, DataTypes);

TYPED_TEST(ParallelizedFCBridgeTest, TestInitialization){
  EXPECT_TRUE(this->ParallelizedConvolutionBridge_);
  EXPECT_TRUE(this->layer1);
  EXPECT_TRUE(this->layer2);
  EXPECT_TRUE(this->ParallelizedFCBridge_);
  EXPECT_TRUE(this->layer1c);
  EXPECT_TRUE(this->layer2c);
}


TYPED_TEST(ParallelizedFCBridgeTest, TestForward){
  // typedef typename TypeParam::T T;

  for(int i=0;i<this->iR*this->iC*this->iD*this->mB;i++){
    this->data1->get_p_data()[i] = float(rand()%100) / 100.0;
    // Set the same input to the fc data cube
    this->data1c->get_p_data()[i] = this->data1->get_p_data()[i];
    this->grad1->get_p_data()[i] = 0;
    this->grad1c->get_p_data()[i] = 0;
  }

  for(int i=0;i<this->k*this->k*this->iD*this->oD;i++){
    this->ParallelizedConvolutionBridge_->p_model_cube->get_p_data()[i] = float(rand()%100) / 100.0;
    // Also copy the model to the FC layer
    this->ParallelizedFCBridge_->p_model_cube->get_p_data()[i]
      = this->ParallelizedConvolutionBridge_->p_model_cube->get_p_data()[i];
  }

  for(int i=0;i<this->oD;i++){
    this->ParallelizedConvolutionBridge_->p_bias_cube->get_p_data()[i] = float(rand()%100) / 100.0;
    this->ParallelizedFCBridge_->p_bias_cube->get_p_data()[i]
      = this->ParallelizedConvolutionBridge_->p_bias_cube->get_p_data()[i];
  }

  this->ParallelizedConvolutionBridge_->forward();

  this->ParallelizedFCBridge_->forward();

  // Verify that the FC layer matches the conv layer
  for (int i=0;i<this->oR*this->oC*this->oD*this->mB;i++) {
    EXPECT_NEAR(this->data2c->get_p_data()[i], this->data2->get_p_data()[i], EPS);
  }
}


TYPED_TEST(ParallelizedFCBridgeTest, TestBackward){
  // typedef typename TypeParam::T T;

  for(int i=0;i<this->iR*this->iC*this->iD*this->mB;i++){
    this->data1->get_p_data()[i] = float(rand()%100) / 100.0;
    this->grad1->get_p_data()[i] = 0;
    // Also set the FC to match the conv
    this->data1c->get_p_data()[i] = this->data1->get_p_data()[i];
    this->grad1c->get_p_data()[i] = this->grad1->get_p_data()[i];
  }

  for(int i=0;i<this->k*this->k*this->iD*this->oD;i++){
    this->ParallelizedConvolutionBridge_->p_model_cube->get_p_data()[i] = float(rand()%100) / 100.0;
    this->ParallelizedFCBridge_->p_model_cube->get_p_data()[i]
      = this->ParallelizedConvolutionBridge_->p_model_cube->get_p_data()[i];
  }

  for(int i=0;i<this->oD;i++){
    this->ParallelizedConvolutionBridge_->p_bias_cube->get_p_data()[i] = float(rand()%100) / 100.0;
    this->ParallelizedFCBridge_->p_bias_cube->get_p_data()[i]
      = this->ParallelizedConvolutionBridge_->p_bias_cube->get_p_data()[i];
  }

  int oR = this->oR;
  int oC = this->oC;

  for (int i=0;i<oR*oC*this->oD*this->mB;i++) {
    this->data2->get_p_data()[i] = 0;
    this->grad2->get_p_data()[i] = i*0.1;
    this->data2c->get_p_data()[i] = 0;
    this->grad2c->get_p_data()[i] = i*0.1;
  }

  this->ParallelizedConvolutionBridge_->forward();
  this->ParallelizedFCBridge_->forward();
  
  this->ParallelizedConvolutionBridge_->backward();
  this->ParallelizedFCBridge_->backward();

  // Verify FC matches Conv

  for (int i=0;i<this->iR*this->iC*this->iD*this->mB;i++) {
    EXPECT_NEAR(this->grad1->get_p_data()[i], this->grad1c->get_p_data()[i], EPS);
  }

  for (int i=0;i<this->oD;i++) {
    EXPECT_NEAR(this->ParallelizedFCBridge_->p_bias_cube->get_p_data()[i],
      this->ParallelizedConvolutionBridge_->p_bias_cube->get_p_data()[i], EPS);
  }

  for (int i=0;i<this->k*this->k*this->iD*this->oD;i++) {
    EXPECT_NEAR(this->ParallelizedFCBridge_->p_model_cube->get_p_data()[i],
      this->ParallelizedConvolutionBridge_->p_model_cube->get_p_data()[i], EPS);
  }
}

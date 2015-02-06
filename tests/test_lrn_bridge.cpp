#include "../src/Kernel.h"
#include "../src/LogicalCube.h"
#include "../src/Layer.h"
#include "../src/config.h"
#include "../src/Connector.h"
#include "../src/bridges/LRNBridge.h"
#include "test_types.h"
#include "gtest/gtest.h"
#include <iostream>
#include <fstream>
#include <assert.h>
#include <cmath>
#include <cstring>

template <typename TypeParam>
class LRNBridgeTest : public ::testing::Test {
 public:
  typedef typename TypeParam::T T;	
  LRNBridgeTest(){
  	data1 = new LogicalCube<T, Layout_CRDB>(iR, iC, iD, mB);
    grad1 = new LogicalCube<T, Layout_CRDB>(iR, iC, iD, mB);
    
    data2 = new LogicalCube<T, Layout_CRDB>(iR, iC, iD, mB);
    grad2 = new LogicalCube<T, Layout_CRDB> (iR, iC, iD, mB);

    layer1 = new Layer<T, Layout_CRDB>(data1, grad1);
    layer2 = new Layer<T, Layout_CRDB>(data2, grad2);
    
    LRNBridge_ = new LRNBridge<T, Layout_CRDB, T, Layout_CRDB>(layer1, layer2, alpha, beta, local_size);
   } 

  	virtual ~LRNBridgeTest() { delete LRNBridge_; delete layer1; delete layer2;}
    LRNBridge<T, Layout_CRDB, T, Layout_CRDB>* LRNBridge_;

  	LogicalCube<T, Layout_CRDB>* data1;
    LogicalCube<T, Layout_CRDB>* grad1;
    
    LogicalCube<T, Layout_CRDB>* data2;
    LogicalCube<T, Layout_CRDB>* grad2;

    Layer<T, Layout_CRDB>* layer1;
    Layer<T, Layout_CRDB>* layer2;

    static const int mB = 6;
    static const int iD = 3;
    static const int iR = 10;
    static const int iC = 10;
    static constexpr float alpha = 0.0001;
    static constexpr float beta = 0.75;
    static const int local_size = 3;
};

typedef ::testing::Types<FloatCRDB> DataTypes;

TYPED_TEST_CASE(LRNBridgeTest, DataTypes);

//openblas_set_num_threads -- undefined reference -- currently disabled
TYPED_TEST(LRNBridgeTest, TestInitialization){
  EXPECT_TRUE(this->LRNBridge_);
  EXPECT_TRUE(this->layer1);
  EXPECT_TRUE(this->layer2);
}

TYPED_TEST(LRNBridgeTest, TestForward){
    srand(1);
	typedef typename TypeParam::T T;
	for(int i=0;i<this->iR*this->iC*this->iD*this->mB;i++){
        this->data1->p_data[i] = rand()%10;
    }

    this->LRNBridge_->forward();

    std::fstream expected_output("lrn_forward.txt", std::ios_base::in);
    
    T output;
    int idx = 0;
    if (expected_output.is_open()) {
        expected_output >> output;
        while (!expected_output.eof()) {
            EXPECT_NEAR(this->data2->p_data[idx], output, EPS);
            expected_output >> output;
            idx++;
        }
    }
    expected_output.close();
}


TYPED_TEST(LRNBridgeTest, TestBackward){
    typedef typename TypeParam::T T;
    
    srand(1);
    for(int i=0;i<this->iR*this->iC*this->iD*this->mB;i++){
        this->data1->p_data[i] = rand()%10;
    }
    
    int oR = this->iR;
    int oC = this->iC;
    
    this->LRNBridge_->forward();

    srand(1);
    for(int i=0;i<oR*oC*this->iD*this->mB;i++){
        this->grad2->p_data[i] = (rand()%10)/10.0;
    }
    
    this->LRNBridge_->backward();

    std::fstream expected_output("lrn_backward.txt", std::ios_base::in);
    T output;
    int idx = 0;
    if (expected_output.is_open()) {
        expected_output >> output;
        while (!expected_output.eof()) {
            EXPECT_NEAR(this->grad1->p_data[idx], output, EPS);
            expected_output >> output;
            idx++;
        }
    }
    expected_output.close();   
}

//TODO: write gpu version
#include "caffe/layers/noise_data_layer.hpp"

namespace caffe {

template <typename Dtype>
void NoiseDataLayer<Dtype>::DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
     const vector<Blob<Dtype>*>& top) {
  
  // Fetch params
  batch_size_ = this->layer_param_.noise_data_param().batch_size();
  channels_ = this->layer_param_.noise_data_param().channels();
  spatial_size_ = this->layer_param_.noise_data_param().spatial_size();
  distribution_ = this->layer_param_.noise_data_param().distribution();
  mu_ = this->layer_param_.noise_data_param().mu();
  sigma_ = this->layer_param_.noise_data_param().sigma();
  min_ = this->layer_param_.noise_data_param().min();
  max_ = this->layer_param_.noise_data_param().max();
  size_ = channels_ * spatial_size_ * spatial_size_;
  
  // Preliminary checks
  CHECK_GT(batch_size_ * size_, 0) <<
      "batch_size, channels, and spatial_size must be specified and"
      " positive in noise_data_param";
  CHECK_GT(sigma_, 0) <<
      "sigma must be specified and positive in noise_data_param";
  CHECK_GT(max_, min_) <<
      "max must be greater than min in noise_data_param";
  CHECK(distribution_.compare("uniform") == 0 || distribution_.compare("gaussian") == 0) <<
      "distribution must be specified and be either \"uniform\" or \"gaussian\"";
  
  // Initialize shapes
  top[0]->Reshape(batch_size_, channels_, spatial_size_, spatial_size_);
  added_data_.Reshape(batch_size_, channels_, spatial_size_, spatial_size_);
  
  data_ = NULL;
  
  // Sync with added_data on gpu
  #ifdef CPU_ONLY
  added_data_.cpu_data();
  std::cout << "Data layer set up. Using CPU version.\n";
  #endif

  // Sync with added_data on cpu
  #ifndef CPU_ONLY
  added_data_.gpu_data();
  std::cout << "Data layer set up. Using GPU version.\n";
  #endif
}

template <typename Dtype>
void NoiseDataLayer<Dtype>::AddNoiseBlob() {
  
  // Some necessary reshaping before anything else
  added_data_.Reshape(batch_size_, channels_, spatial_size_, spatial_size_);

  // Add noise data
  if (distribution_.compare("uniform") == 0) {
    #ifdef CPU_ONLY
    caffe_rng_uniform(added_data_.count(), Dtype(min_), Dtype(max_), added_data_.mutable_cpu_data());
    #endif
    #ifndef CPU_ONLY
    caffe_gpu_rng_uniform(added_data_.count(), Dtype(min_), Dtype(max_), added_data_.mutable_cpu_data());
    #endif
  } else if (distribution_.compare("gaussian") == 0) {
    #ifdef CPU_ONLY
    caffe_rng_gaussian(added_data_.count(), Dtype(mu_), Dtype(sigma_), added_data_.mutable_cpu_data());
    #endif
    #ifndef CPU_ONLY
    caffe_gpu_rng_gaussian(added_data_.count(), Dtype(mu_), Dtype(sigma_), added_data_.mutable_cpu_data());
    #endif
  }
        
  // Some checks and pointer transferring
  #ifdef CPU_ONLY
  Dtype* top_data = added_data_.mutable_cpu_data();
  std::cout << "Noise added. Using CPU version.\n";
  #endif
  #ifndef CPU_ONLY
  Dtype* top_data = added_data_.mutable_gpu_data();
  std::cout << "Noise added. Using GPU version.\n";
  #endif
  Reset(top_data);
}

template <typename Dtype>
void NoiseDataLayer<Dtype>::Reset(Dtype* data) {
  
  CHECK(data);
  
  data_ = data;
}

template <typename Dtype>
void NoiseDataLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {

  AddNoiseBlob();

  CHECK(data_) << "NoiseDataLayer needs to be initialized by calling Reset";
  
  top[0]->Reshape(batch_size_, channels_, spatial_size_, spatial_size_);
  top[0]->set_cpu_data(data_);
}

#ifdef CPU_ONLY
STUB_GPU(NoiseDataLayer);
#endif

INSTANTIATE_CLASS(NoiseDataLayer);
REGISTER_LAYER_CLASS(NoiseData);

}  // namespace caffe

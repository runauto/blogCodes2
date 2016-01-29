#include "car_benchmark.hpp"
#include "utility.hpp"

#include <ocv_libs/file/utility.hpp>
#include <ocv_libs/tiny_cnn/trainer.hpp>

#include <boost/progress.hpp>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>
#include <iostream>

using namespace tiny_cnn;
using namespace tiny_cnn::activation;

namespace{

cv::ImreadModes const imread_modes = cv::IMREAD_GRAYSCALE;

template<typename Net>
void create_lenet(Net &nn)
{
    using activate = relu;
    /*nn << convolutional_layer<activate>(32, 32, 5, 1, 6)
       << average_pooling_layer<activate>(28, 28, 6, 2)
       << convolutional_layer<activate>(14, 14, 5, 6, 16)
       << average_pooling_layer<activate>(10, 10, 16, 2)
       << convolutional_layer<activate>(5, 5, 5, 16, 120)
       << fully_connected_layer<softmax>(120, 2);//*/

    nn << convolutional_layer<activate>(32, 32, 3, 1, 6, padding::same)
       << max_pooling_layer<activate>(32, 32, 6, 2)
       << convolutional_layer<activate>(16, 16, 3, 6, 16, padding::same)
       << max_pooling_layer<activate>(16, 16, 16, 2)
       << convolutional_layer<activate>(8, 8, 5, 16, 24)
       << fully_connected_layer<softmax>(384, 10);//*/

    /*nn << convolutional_layer<activate>(32, 32, 3, 1, 6, padding::same)
       << max_pooling_layer<activate>(32, 32, 6, 2)
       << convolutional_layer<activate>(16, 16, 3, 6, 16, padding::same)
       << max_pooling_layer<activate>(16, 16, 16, 2)
       << convolutional_layer<activate>(8, 8, 3, 16, 24, padding::same)
       << max_pooling_layer<activate>(8, 8, 24, 2)
       << convolutional_layer<activate>(4, 4, 3, 24, 32, padding::same)
       << fully_connected_layer<softmax>(512, 2);//*/
}

template<typename Net>
void create_minivgg(Net &nn)
{
    using activate = relu;
    nn << convolutional_layer<activate>(32, 32, 3, 3, 32, padding::same)
       << convolutional_layer<activate>(32, 32, 3, 32, 32, padding::same)
       << max_pooling_layer<activate>(32, 32, 32, 2)
       << dropout_layer(16*16*32, 0.25)
       << convolutional_layer<activate>(16, 16, 3, 32, 64, padding::same)
       << convolutional_layer<activate>(16, 16, 3, 64, 64, padding::same)
       << max_pooling_layer<activate>(16, 16, 64, 2)
       << dropout_layer(8*8*64, 0.25)
       << fully_connected_layer<softmax>(8*8*64, 10);
}


}

car_benchmark::car_benchmark()
{            
    load_car_data("train_cars", load_car_region("cars_train_annos.txt"),
                  train_images_, train_labels_);
    load_car_data("test_cars", load_car_region("cars_test_annos.txt"),
                  test_images_, test_labels_);

    load_data("non_cars", 1,
              train_images_, train_labels_);
    load_data("test_non_cars", 1,
              test_images_, test_labels_);//*/

    /*parse_cifar10("cifar-10-batches-bin/data_batch_1.bin", &train_images_, &train_labels_, 0, 1, 0, 0);
    parse_cifar10("cifar-10-batches-bin/data_batch_2.bin", &train_images_, &train_labels_, 0, 1, 0, 0);
    parse_cifar10("cifar-10-batches-bin/data_batch_3.bin", &train_images_, &train_labels_, 0, 1, 0, 0);
    parse_cifar10("cifar-10-batches-bin/data_batch_4.bin", &train_images_, &train_labels_, 0, 1, 0, 0);
    parse_cifar10("cifar-10-batches-bin/data_batch_5.bin", &train_images_, &train_labels_, 0, 1, 0, 0);
    parse_cifar10("cifar-10-batches-bin/test_batch.bin", &test_images_, &test_labels_, 0, 1, 0, 0);//*/

    std::vector<std::pair<vec_t, label_t>> img_and_labels;
    for(size_t i = 0; i != train_images_.size(); ++i){
        img_and_labels.emplace_back(std::move(train_images_[i]),
                                    train_labels_[i]);
    }
    std::random_device rd;
    std::default_random_engine g(rd());
    std::shuffle(std::begin(img_and_labels), std::end(img_and_labels), g);
    for(size_t i = 0; i != train_images_.size(); ++i){
        train_images_[i] = std::move(img_and_labels[i].first);
        train_labels_[i] = img_and_labels[i].second;
    }

    try{
        train_test();
    }catch(std::exception const &ex){
        std::cout<<ex.what()<<std::endl;
    }
}

void car_benchmark::add_data(label_t label, cv::Mat const &img,
                             Labels &labels, Imgs &imgs)
{
    if(!img.empty()){
        cv::Mat resize_img;
        cv::resize(img, resize_img, {32,32});
        imgs.emplace_back(cvmat_to_img<vec_t>(resize_img, -1, 1));
        labels.emplace_back(label);

        //cv::flip(resize_img, resize_img, 0); //flip horizontal
        //imgs.emplace_back(cvmat_to_img<vec_t>(resize_img, 0, 1));
        //labels.emplace_back(label);
    }
}

void car_benchmark::load_car_data(const std::string &folder,
                                  std::map<std::string, cv::Rect> const &car_region,
                                  Imgs &imgs, Labels &labels)
{
    auto const names = ocv::file::get_directory_files(folder);
    int const expand_pix = 16;
    for(auto const &name : names){
        cv::Mat img = cv::imread(folder + "/" + name,
                                 imread_modes);
        auto rect = car_region.at(name);
        rect.x = std::max(0, rect.x - expand_pix);
        rect.width = std::min(rect.width + 2*expand_pix,
                              img.cols - rect.x - 1);

        rect.y = std::max(0, rect.y - expand_pix);
        rect.height = std::min(rect.height + 2*expand_pix,
                               img.rows - rect.y - 1);

        add_data(0, img(rect), labels, imgs);
    }
}

std::map<std::string, cv::Rect> car_benchmark::
load_car_region(const std::string &file) const
{
    std::ifstream in(file);
    int x1,x2,y1,y2;
    std::string name;
    std::map<std::string, cv::Rect> result;
    while(!in.eof()){
        in>>x1>>x2>>y1>>y2>>name;
        result.insert({name, {x1, y1, x2-x1, y2-y1}});
    }

    return result;
}

void car_benchmark::load_data(std::string const &folder,
                              label_t label,
                              Imgs &imgs, Labels &labels)
{
    auto const names = ocv::file::get_directory_files(folder);
    for(auto const &name : names){
        cv::Mat img = cv::imread(folder + "/" + name,
                                 imread_modes);
        add_data(label, img, labels, imgs);
    }
}

void car_benchmark::train_test()
{
    network<mse, adagrad> nn;

    create_lenet(nn);

    int const minibatch_size = 20;
    int const num_epochs = 10;

    nn.optimizer().alpha *= std::sqrt(minibatch_size);

    ocv::tiny_cnn::trainer tt("car_weights", minibatch_size, num_epochs);
    tt.train_and_test(nn, train_images_, train_labels_,
                      test_images_, test_labels_);
}
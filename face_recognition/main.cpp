#include "face_capture.hpp"
#include "face_detector.hpp"
#include "face_recognition.hpp"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <iostream>

using vmap = boost::program_options::variables_map;

int capture_face(vmap const &command);
int images_record(vmap const &command);
int recognize_face(vmap const &command);

vmap parse_command_line(int argc, char **argv);

int main(int argc, char **argv)
{
    try{
        auto const command = parse_command_line(argc, argv);
        if(command.count("capture")){
            int const mode = command["capture"].as<int>();
            if(mode == 0){
                return capture_face(command);
            }else if(mode == 1){
                return recognize_face(command);
            }else{
                return images_record(command);
            }
        }
    }catch(std::exception const &ex){
        std::cout<<ex.what()<<std::endl;
    }
}

int capture_face(vmap const &command)
{
    if(!command.count("output_folder")){
        std::cout<<"must specify output_folder"<<std::endl;
        return -1;
    }

    face_capture capture(command["output_folder"].as<std::string>());
    return capture.capture();
}

vmap parse_command_line(int argc, char **argv)
{
    using namespace boost::program_options;

    try
    {
        options_description desc{"Options"};
        desc.add_options()
                ("help,h", "Help screen")
                ("capture,c", value<int>()->default_value(1), "0 will enter capture mode; "
                                                              "1 will enter recogniton mode;"
                                                              "2 will enter video record mode"
                                                              "default value is 1")
                ("input_folder,i", value<std::string>(), "Specify the input folders")
                ("record_folder,v", value<std::string>(), "Specify the folder of record files")
                ("output_folder,o", value<std::string>(), "Specify the output folder")
                ("random_size,r", value<size_t>()->default_value(0), "Specify the random size of faces");

        variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);

        notify(vm);

        if (vm.count("help")){
            std::cout << desc << std::endl;
        }

        return vm;
    }
    catch (const error &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    return {};
}

int images_record(vmap const &command)
{
    if(!command.count("record_folder")){
        std::cout<<"must specify record_video"<<std::endl;
        return -1;
    }

    cv::VideoCapture cam(0);
    if (!cam.isOpened()){
        std::cout <<"Could not open the camera"<<std::endl;
        return -1;
    }

    auto const rfolder = command["record_folder"].as<std::string>();
    if(!boost::filesystem::exists(rfolder)){
        if(!boost::filesystem::create_directory(rfolder)){
            std::cout<<"cannot create folder"<<std::endl;
            return -1;
        }
    }

    cv::Mat frame;
    size_t index = 0;
    while(true){
        cam.read(frame);
        if(!frame.empty()){
            cv::imshow("record mode", frame);
            std::ostringstream ostream;
            ostream <<rfolder<<"/"<<std::setw(5)<<std::setfill('0')
                   <<std::to_string(index++)<<".jpg";
            cv::imwrite(ostream.str(),frame);
            int const key = cv::waitKey(30);
            if(key == 'q'){
                break;
            }
        }else{
            break;
        }
    }

    return 0;
}


int recognize_face(vmap const &command)
{
    if(!command.count("input_folder")){
        std::cout<<"must specify input_folder"<<std::endl;
        return -1;
    }

    auto const input_folder = command["input_folder"].as<std::string>();
    auto const random_size = command["random_size"].as<size_t>();

    cv::VideoCapture cap(0);
    if(!cap.isOpened()){
        std::cout<<"cannot open camera"<<std::endl;
        return -1;
    }

    face_recognition fr(input_folder, random_size);
    face_detector fd;
    cv::Mat frame;
    while(true){
        cap.read(frame);
        if(!frame.empty()){
            auto const &regions = fd.detect(frame);
            for(size_t i = 0; i != regions.size(); ++i){
                auto const name = fr.recognize(frame(regions[i]));
                cv::Point const point = {std::max(0, regions[i].x - regions[i].x/5),
                                         std::max(0, regions[i].y - 30)};
                if(!name.empty()){
                    cv::rectangle(frame, regions[i], {0,255,0}, 2);
                    cv::putText(frame, name, point,
                                cv::FONT_HERSHEY_COMPLEX, 1.0, {0,255,0}, 2);
                }else{
                    cv::putText(frame, "unknown", point,
                                cv::FONT_HERSHEY_COMPLEX, 1.0, {0,0,255}, 2);
                    cv::rectangle(frame, regions[i], {0,0,255}, 2);
                }
            }
            cv::imshow("recognize mode", frame);
            int const key = cv::waitKey(30);
            if(key == 'q'){
                break;
            }
        }else{
            break;
        }
    }

    return 0;
}

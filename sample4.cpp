#include <opencv2/opencv.hpp>
#include <stdio.h>

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define FRAME_RATE 14

int main(int argc, char *argv[])
{
    int INIT_TIME = 50;
    double B_PARAM = 1.0 / 50.0;
    double T_PARAM = 1.0 / 200.0;
    double Zeta = 10.0;

    cv::VideoCapture cap;
    cv::Mat frame;
    cv::Mat avg_img, sgm_img;
    cv::Mat lower_img, upper_img, tmp_img;
    cv::Mat dst_img, msk_img;

    cv::Mat fruits = cv::imread("fruits.jpg");

    // 1. initialize VideoCapture
    if(argc > 1){
        std::string in_name = argv[1];
        if(in_name.size() == 1 && isdigit(in_name[0])){
            cap.open(in_name[0] - '0'); // open with device id
        }else{
            cap.open(in_name); // open with filename
        }
    }else{
        cap.open(0);
    }

    cap.set(cv::CAP_PROP_FPS, FRAME_RATE);
    cap.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

    if(!cap.isOpened()){
        printf("Cannot open the video.\n");
        exit(0);
    }

    // 2. prepare window for showing images
    cv::namedWindow("Input", 1);
    cv::namedWindow("FG", 1);
    cv::namedWindow("mask", 1);

    // 3. calculate initial value of background
    cap >> frame;

    cv::Size s = frame.size();

    // ---変更: fruits.jpg を検証し、フレームと同じサイズ・チャンネルに揃える---
    if (fruits.empty()) {
        printf("ERROR: cannot load fruits.jpg (背景画像を読み込めません)\n");
        return 0;
    }
    // チャンネル数をBGR(3ch)へ正規化
    if (fruits.channels() == 4) {
        cv::cvtColor(fruits, fruits, cv::COLOR_BGRA2BGR);
    } else if (fruits.channels() == 1) {
        cv::cvtColor(fruits, fruits, cv::COLOR_GRAY2BGR);
    }
    // サイズをフレームと一致させる
    if (fruits.size() != s) {
        cv::resize(fruits, fruits, s);
    }

    avg_img.create(s, CV_32FC3);
    sgm_img.create(s, CV_32FC3);
    lower_img.create(s, CV_32FC3);
    upper_img.create(s, CV_32FC3);
    tmp_img.create(s, CV_32FC3);

    dst_img.create(s, CV_8UC3);
    msk_img.create(s, CV_8UC1);

    printf("Background statistics initialization start\n");

    avg_img = cv::Scalar(0, 0, 0);

    for(int i = 0; i < INIT_TIME; i++){
        cap >> frame;
        cv::Mat tmp;
        frame.convertTo(tmp, avg_img.type());
        cv::accumulate(tmp, avg_img);
    }

    avg_img.convertTo(avg_img, -1, 1.0 / INIT_TIME);
    sgm_img = cv::Scalar(0, 0, 0);

    //標準偏差を作る
    for(int i = 0; i < INIT_TIME; i++){
        cap >> frame;
        frame.convertTo(tmp_img, avg_img.type());
        cv::subtract(tmp_img, avg_img, tmp_img);
        cv::pow(tmp_img, 2.0, tmp_img);
        tmp_img.convertTo(tmp_img, -1, 2.0);
        cv::sqrt(tmp_img, tmp_img);
        cv::accumulate(tmp_img, sgm_img);
    }

    sgm_img.convertTo(sgm_img, -1, 1.0 / INIT_TIME);

    printf("Background statistics initialization finish\n");

    bool loop_flag = true;
    while(loop_flag){
        cap >> frame;
        frame.convertTo(tmp_img, tmp_img.type());

        // 4. check whether pixels are background or not
        cv::subtract(avg_img, sgm_img, lower_img);
        cv::subtract(lower_img, Zeta, lower_img);
        cv::add(avg_img, sgm_img, upper_img);
        cv::add(upper_img, Zeta, upper_img);
        cv::inRange(tmp_img, lower_img, upper_img, msk_img);

        // 5. recalculate
        cv::subtract(tmp_img, avg_img, tmp_img);
        cv::pow(tmp_img, 2.0, tmp_img);
        tmp_img.convertTo(tmp_img, -1, 2.0);
        cv::sqrt(tmp_img, tmp_img);

        // 6. renew avg img and sgm img
        cv::accumulateWeighted(frame, avg_img, B_PARAM, msk_img);
        cv::accumulateWeighted(tmp_img, sgm_img, B_PARAM, msk_img);

        cv::bitwise_not(msk_img, msk_img);
        cv::accumulateWeighted(tmp_img, sgm_img, T_PARAM, msk_img);

        //dst_img = cv::Scalar(0);
        fruits.copyTo(dst_img);
        frame.copyTo(dst_img, msk_img);

        cv::imshow("Input", frame);
        cv::imshow("FG", dst_img);
        cv::imshow("mask", msk_img);

        char key = cv::waitKey(10);
        if(key == 27){
            loop_flag = false;
        }
    }
    return 0;
}
#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "UTM.h"
#include "transform.h"
#include <iomanip>
#include "opencv2/opencv.hpp"
#include <boost/filesystem.hpp>

using namespace std;
using namespace cv;

class PointUTM{
public:
    PointUTM(double a, double b){
        x = a;
        y = b;
    }
    double x;
    double y;
};
class PointGPS{
public:
    double lon;
    double lat;
};

vector<PointUTM> acquireRoadPoint(string file_name){
    std::ifstream inmemory;
    inmemory.open(file_name);
    if(!inmemory.is_open()){
        cout << "File open fail!\n";
        return vector<PointUTM> {};
    }
    string s;
    vector<PointUTM> res;
    while(std::getline(inmemory, s)){
        vector<string> line;
        boost::split(line, s, boost::is_any_of(" "));
        double UTMNorthing = 0;
        double UTMEasting = 0;
        int UTMZone;
        XY_LB::Mercator::LB2XY(std::stod(line.at(1)), std::stod(line.at(2)), UTMEasting, UTMNorthing);
        std::cout << std::setprecision(12) << UTMEasting << ", " << std::setprecision(12) << UTMNorthing << std::endl;
        res.emplace_back(UTMEasting, UTMNorthing);
    }
    inmemory.close();
    return res;
}

std::vector<PointUTM> getTiffTopLeft(const char* pszFilename){
    GDALDataset  *poDataset;
    GDALAllRegister();
    poDataset = (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );
    double adfGeoTransform[6];
    if( poDataset == NULL )
    {
        std::cout << "Not found tiff file\n";
    }
    else{
//        printf( "Driver: %s/%s\n",
//                poDataset->GetDriver()->GetDescription(),
//                poDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
//        printf( "Size is %dx%dx%d\n",
//                poDataset->GetRasterXSize(), poDataset->GetRasterYSize(),
//                poDataset->GetRasterCount() );
//        if( poDataset->GetProjectionRef()  != NULL )
//            printf( "Projection is `%s'\n", poDataset->GetProjectionRef() );
        if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None ){
            printf( "Origin = (%.6f,%.6f)\n",
                    adfGeoTransform[0], adfGeoTransform[3] );
            printf( "Pixel Size = (%.6f,%.6f)\n",
                    adfGeoTransform[1], adfGeoTransform[5] );
        }
    }
    return std::vector<PointUTM>{PointUTM{adfGeoTransform[0], adfGeoTransform[3]}, PointUTM{adfGeoTransform[1], adfGeoTransform[5]}};
}

std::vector<PointUTM> getTiffTopLeft(std::string pszFilename){

    char *pszFilename_ctr = new char[pszFilename.length() + 1];
    strcpy(pszFilename_ctr, pszFilename.c_str());

    GDALDataset  *poDataset;
    GDALAllRegister();
    poDataset = (GDALDataset *) GDALOpen( pszFilename_ctr, GA_ReadOnly );
    double adfGeoTransform[6];
    if( poDataset == NULL )
    {
        std::cout << "Not found tiff file\n";
    }
    else{
        if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None ){
            printf( "Origin = (%.6f,%.6f)\n",
                    adfGeoTransform[0], adfGeoTransform[3] );
            printf( "Pixel Size = (%.6f,%.6f)\n",
                    adfGeoTransform[1], adfGeoTransform[5] );
        }
    }
    delete [] pszFilename_ctr;
    return std::vector<PointUTM>{PointUTM{adfGeoTransform[0], adfGeoTransform[3]}, PointUTM{adfGeoTransform[1], adfGeoTransform[5]}};
}


int main()
{
    /// combine two image to one
    const char *ground_img = "/home/ugv-yu/bryan/test/geotiff/alashanNorth.tif";
    auto topLeftPoint_ground = getTiffTopLeft(ground_img);
    auto groudImg = cv::imread(ground_img, 1);
    cout << "ground img size: " << groudImg.size().width << ", " << groudImg.size().height << std::endl;

    const char* tif_dir = "/home/ugv-yu/bryan/test/geotiff/TIF";
    std::vector<boost::filesystem::directory_entry> tiff_str; // To save the file names in a vector.
    if(boost::filesystem::is_directory(tif_dir)) {
        std::copy(boost::filesystem::directory_iterator(tif_dir), boost::filesystem::directory_iterator(),
                  back_inserter(tiff_str));
        std::cout << tif_dir << " is a directory containing:\n";
    }
    else{
        std::cerr << "directory not accessable\n";
        return -1;
    }
    double bigResolution = topLeftPoint_ground.at(1).x;
    for ( std::vector<boost::filesystem::directory_entry>::const_iterator it = tiff_str.begin();
          it != tiff_str.end();  ++ it )
    {
        auto tif_name = it->path().string();
        auto topLeftPoint_odm = getTiffTopLeft(tif_name);
        cv::Mat smallImg = cv::imread(tif_name, 1);
        cout << "img size: (" << smallImg.size().width << ", " << smallImg.size().height << ")" << std::endl;

        /// smallImg: x, y range -> bigImg pixel range
        int width_in_bigImg = static_cast<int>(smallImg.size().width * topLeftPoint_odm.at(1).x / topLeftPoint_ground.at(1).x);
        int height_in_bigImg = static_cast<int>(smallImg.size().height * topLeftPoint_odm.at(1).y / topLeftPoint_ground.at(1).y);
        cout << "small img within big in pixel: " << width_in_bigImg << ", " << height_in_bigImg << std::endl;
        /// smallImg resize to above range then mask
        cv::Mat smallImg_in_bigImg;
        cv::resize(smallImg,smallImg_in_bigImg, cv::Size(width_in_bigImg, height_in_bigImg), 0 ,0);
        cv::imshow("img", smallImg_in_bigImg);
        cv::waitKey(0);

        int zone = 48;
        double lat_topleft = 0;
        double lon_topleft = 0;
        UTMXYToLatLon(topLeftPoint_odm.at(0).x, topLeftPoint_odm.at(0).y, zone, false,lat_topleft, lon_topleft);
        cout << "UTM is " << topLeftPoint_odm.at(0).x << ", " << topLeftPoint_odm.at(0).y << std::endl;
        cout << "lat & lon is: " << std::setprecision(12) << RadToDeg(lat_topleft) << ", " << RadToDeg(lon_topleft) << std::endl;

        double x_topleft_in_ground = 0;
        double y_topleft_in_ground = 0;
        XY_LB::Mercator::LB2XY(RadToDeg(lon_topleft), RadToDeg(lat_topleft), x_topleft_in_ground, y_topleft_in_ground);
        cout << "x & y_topleft_in_ground: " << x_topleft_in_ground << ", " << y_topleft_in_ground << std::endl;

        auto pixel_x = (x_topleft_in_ground - topLeftPoint_ground.at(0).x) / topLeftPoint_ground.at(1).x;
        auto pixel_y = (y_topleft_in_ground - topLeftPoint_ground.at(0).y) / topLeftPoint_ground.at(1).y;
        std::cout << pixel_x << ", " << pixel_y << std::endl;

        auto rec = cv::Rect(static_cast<int>(pixel_x), static_cast<int>(pixel_y),
                            smallImg_in_bigImg.size().width, smallImg_in_bigImg.size().height);

        smallImg_in_bigImg.copyTo(groudImg(rec));
    }


//    /// read road points
//    string roadFile = "/home/ugv-yu/bryan/test/geotiff/paowei_way.txt";
//    auto roadPoints = acquireRoadPoint(roadFile);
//
//    /// read road network
//    string roadNet = "/home/ugv-yu/bryan/test/geotiff/alashanroad.txt";
//    auto roadNetPoints = acquireRoadPoint(roadNet);
//
//    /// read first day hole points
//    string roadFirstDay = "/home/ugv-yu/bryan/test/geotiff/alashan.txt";
//    auto firstDayPoints = acquireRoadPoint(roadFirstDay);

//    for(const auto & point: roadPoints){
//        auto pi_x = (point.x - topLeftPoint_ground.at(0).x) / topLeftPoint_ground.at(1).x;
//        auto pi_y = (point.y - topLeftPoint_ground.at(0).y) / topLeftPoint_ground.at(1).y;
//        cv::circle(groudImg,cv::Point(pi_x, pi_y), 1, cv::Scalar(0,25,255), -1); /// smallImg: x, y range -> bigImg pixel range
//    }
//
//    for(const auto & point: roadNetPoints){
//        auto pi_x = (point.x - topLeftPoint_ground.at(0).x) / topLeftPoint_ground.at(1).x;
//        auto pi_y = (point.y - topLeftPoint_ground.at(0).y) / topLeftPoint_ground.at(1).y;
//        cv::circle(groudImg,cv::Point(pi_x, pi_y), 1, cv::Scalar(255,25,0), -1);
//    }
//
//    for(const auto & point: firstDayPoints){
//        auto pi_x = (point.x - topLeftPoint_ground.at(0).x) / topLeftPoint_ground.at(1).x;
//        auto pi_y = (point.y - topLeftPoint_ground.at(0).y) / topLeftPoint_ground.at(1).y;
//        cv::circle(groudImg,cv::Point(pi_x, pi_y), 1, cv::Scalar(0,255,0), -1);
//    }
//    cv::resize(groudImg, groudImg, cv::Size(0,0), 0.1,0.1);
//    cv::imshow("res", groudImg);
    cv::imwrite("three.jpg", groudImg);
//    cv::imshow("smallImg", groudImg(rec));
    cv::waitKey(100);
    return 0;
}

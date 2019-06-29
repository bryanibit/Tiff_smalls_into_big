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
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include <tuple>

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
        LatLonToUTMXY(std::stod(line.at(2)), std::stod(line.at(1)), UTMZone, UTMEasting, UTMNorthing);
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

int main()
{
    const char *odm_img = "/home/ugv-yu/bryan/test/geotiff/odm_orthophoto.tif";
    auto topLeftPoint_odm = getTiffTopLeft(odm_img);
    cv::Mat smallImg = cv::imread(odm_img, 1);
    cout << "img size: (" << smallImg.size().width << ", " << smallImg.size().height << ")" << std::endl;

    // combine two image to one
    const char *ground_img = "/home/ugv-yu/bryan/test/geotiff/alashanNorth.tif";
    auto topLeftPoint_ground = getTiffTopLeft(ground_img);
    auto groudImg = cv::imread(ground_img, 1);
    cout << "ground img size: " << groudImg.size().width << ", " << groudImg.size().height << std::endl;

    // smallImg: x, y range -> bigImg pixel range
    double bigResolution = topLeftPoint_ground.at(1).x;
    int width_in_bigImg = static_cast<int>(smallImg.size().width * topLeftPoint_odm.at(1).x / topLeftPoint_ground.at(1).x);
    int height_in_bigImg = static_cast<int>(smallImg.size().height * topLeftPoint_odm.at(1).y / topLeftPoint_ground.at(1).y);
    cout << "small img within big in pixel: " << width_in_bigImg << ", " << height_in_bigImg << std::endl;
    // smallImg resize to above range then mask
    cv::Mat smallImg_in_bigImg;
    cv::resize(smallImg,smallImg_in_bigImg, cv::Size(width_in_bigImg, height_in_bigImg), 0 ,0);
    cv::imshow("img", smallImg_in_bigImg);
    cv::waitKey(0);

//    auto topRightBig = PointUTM(11754466.9511462640, 4723788.790894158);
//    auto pixel_x = (topLeftPoint_odm.at(0).x- topRightBig.x) / bigResolution;
//    auto pixel_y = ((topRightBig.y + groudImg.size().height * bigResolution)- topLeftPoint_odm.at(0).y) / bigResolution;
    int zone = 48;
//    double x_=0;  double y_ = 0; double test_lat = 39.03767;double test_lon = 105.60;
//    LatLonToUTMXY(test_lat,test_lon,zone,x_, y_);
//    cout << "zone: " << zone << std::endl;
//    cout << "test_lon_lat: " << test_lat << ", " << test_lon << std::endl;
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
    cv::imwrite("res.jpg", groudImg);
    cv::waitKey(100);
    return 0;
}

#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "UTM.h"
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
    auto file_point = acquireRoadPoint("/home/ugv-yu/bryan/test/geotiff/alashan.txt");
    auto topLeftPoint = getTiffTopLeft("/home/ugv-yu/bryan/test/geotiff/odm_orthophoto.tif");
    cv::Mat displayImg = cv::imread("/home/ugv-yu/bryan/test/geotiff/odm_orthophoto.tif", 1);
    cout << "img size: (" << displayImg.size().width << ", " << displayImg.size().height << ")" << std::endl;

//    vector<cv::Point2d> pixelPoints;
    for(const auto pointxy: file_point){
        std::cout << "x-> " << (pointxy.x - topLeftPoint.at(0).x) / topLeftPoint.at(1).x << std::endl;
        std::cout << "y-> " << (pointxy.y - topLeftPoint.at(0).y) / topLeftPoint.at(1).y << std::endl;
        std::cout << "\n";
//        pixelPoints.emplace_back((pointxy.x - topLeftPoint.at(0).x) / topLeftPoint.at(1).x,
//                                 (pointxy.y - topLeftPoint.at(0).y) / topLeftPoint.at(1).y);
        cv::circle(displayImg, cv::Point((pointxy.x - topLeftPoint.at(0).x) / topLeftPoint.at(1).x,
                                         (pointxy.y - topLeftPoint.at(0).y) / topLeftPoint.at(1).y), 3, cv::Scalar(0,25,255), -1);
    }
    cv::imshow("img", displayImg);
    cv::waitKey(0);
    return 0;
}
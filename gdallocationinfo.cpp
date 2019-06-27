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
    return res;
}

PointUTM getTiffTopLeft(const char* pszFilename){
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
    return PointUTM{adfGeoTransform[0], adfGeoTransform[3]};
}

int main()
{
    auto file_point = acquireRoadPoint("/home/ugv-yu/bryan/test/geotiff/alashan.txt");
    auto topLeftPoint = getTiffTopLeft("/home/ugv-yu/bryan/test/geotiff/odm_orthophoto.tif");
//    cv::Mat displayImg = cv::imread("/home/ugv-yu/bryan/test/geotiff/odm_orthophoto.tif", 1);
//    for(const auto &[east, north]: file_point){
//
//    }
    return 0;
}

#ifndef XY2000_LB2000_H
#define XY2000_LB2000_H

#include <math.h>

namespace XY_LB {
    class S2000{
    public:
        /**
         * @brief XY2000_LB2000
         * @param y0
         * @param x0
         * @param L
         * @param B
         * @param z_zone
         */
         static void XY2LB(double y0, double x0, double& L, double& B, int z_zone);
         /**
         * @brief LB2000_XY2000
         * @param L
         * @param B
         * @param globalX
         * @param globalY
         */
         static void LB2XY(double L, double B, double &globalX, double &globalY);
};

    class Mercator{
    public:
        /**
         * @brief XY2LB, Mercator Projection, from global x-y coordinate to lon-lat coordinate
         * @param x, global x, m
         * @param y, global y, m
         * @param L, longitude,degree
         * @param B, latitude, degree
         */
        static void XY2LB(double x, double y, double& L, double& B);

        /**
         * @brief LB2XY, Mercator Projection, from lon-lat coordinate to global x-y coordinate
         * @param L, longitude, degree
         * @param B, latitude, degree
         * @param x, global x, m
         * @param y, global y, m
         */
        static void LB2XY(double L, double B, double& x, double& y);

    };
}

#endif // XY2000_LB2000_H

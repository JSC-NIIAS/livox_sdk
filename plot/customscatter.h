#ifndef CUSTOMSCATTER_H
#define CUSTOMSCATTER_H

#include <Q3DScatter>
#include <QWidget>
#include <QObject>
#include <QList>

#include "livox_def.h"

#include "qt_vinvoke.h"

using namespace QtDataVisualization;

//=======================================================================================

Q_DECLARE_METATYPE( QList<QVector3D> )
Q_DECLARE_METATYPE( QList<LivoxRawPoint> )

//=======================================================================================

struct DrawPloperty
{
    QString      legend_name;
    std::string  fn_name;

    QColor       color;

    bool need_new_graph = false;
    bool need_replot = false;
    bool need_gr_clear = false;
    bool need_leg_clear = false;
    bool need_match = false;
    bool need_track = false;
    bool need_lidar = false;
    bool need_switch = false;
    bool need_balise = false;
    bool check_cam_dist = false;
    bool has_ground = true;

    int layer = 0;
};

Q_DECLARE_METATYPE( DrawPloperty )

//=======================================================================================
class CustomScatter : public Q3DScatter
{
        Q_OBJECT

    public:

        CustomScatter();

        ~CustomScatter();

        //-------------------------------------------------------------------------------

    public slots:

        void plot_pnts( const QList<LivoxRawPoint>& pnts, const DrawPloperty& d_prop );

        //-------------------------------------------------------------------------------

    private:

        QScatter3DSeries* _layer;
        QScatterDataArray _data;

        //-----------------------------------------------------------------------------------

        using LidarColors = QList<QColor>;

        LidarColors qcolors
        {
            QColor( Qt::black    ),
                    QColor( Qt::blue   ),
                    QColor( Qt::red    ),
                    QColor( Qt::green )
        };

        //-----------------------------------------------------------------------------------

        void replot();

        void clear_layer( const int _layer );

        void clear_layers();

        void clear_series();

        void fill_layer( const QScatterDataArray& data, QScatter3DSeries& _layer );

};

//=======================================================================================

#endif // CUSTOMSCATTER_H

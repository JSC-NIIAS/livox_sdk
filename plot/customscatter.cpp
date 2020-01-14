#include "customscatter.h"

using namespace QtDataVisualization;

//=======================================================================================
CustomScatter::CustomScatter()
{
    this->setFlags( this->flags() ^ Qt::FramelessWindowHint );

    this->setPosition( { 900, 10 } );
    this->setWidth( 700 );
    this->setHeight( 700 );

    this->axisX()->setTitle("X");
    this->axisY()->setTitle("Y");
    this->axisZ()->setTitle("Z");

    this->axisX()->setRange( - 20, 20 );
    this->axisY()->setRange( 0, 200 );
    this->axisZ()->setRange( 0, 6 );

    this->setShadowQuality( QAbstract3DGraph::ShadowQualityNone );

    //    auto preset = Q3DCamera::CameraPresetBehind;
    //    this->scene()->activeCamera()->setCameraPreset( ( Q3DCamera::CameraPreset ) preset );

    this->scene()->activeCamera()->setCameraPosition( - 2, 5, 250.0f );

    this->show();
}
//=======================================================================================
CustomScatter::~CustomScatter()
{
    if ( _layer != nullptr )
        this->removeSeries( _layer );

    this->close();

    this->destroy();
}
//=======================================================================================


//=======================================================================================
void CustomScatter::plot_pnts( const QList<LivoxRawPoint>& pnts,
                               const DrawPloperty& d_prop )
{
    if ( pnts.empty() )
        return;

    if ( d_prop.need_gr_clear )
        clear_series();

    QScatter3DSeries* tmp = new QScatter3DSeries;

    QScatterDataArray data;

    for ( const auto& pnt: pnts )
        data << QVector3D( pnt.x / 100, pnt.y / 100, pnt.z / 100 );

    tmp->dataProxy()->addItems( data );
    tmp->setBaseColor( qcolors.at( 1 ) );
    tmp->setItemSize( 0.05 );
    tmp->setMesh( QAbstract3DSeries::MeshPoint );

    this->addSeries( tmp );

    replot();
}
//=======================================================================================


//=======================================================================================
void CustomScatter::replot()
{
    this->show();
}
//=======================================================================================
void CustomScatter::clear_layer( const int layer )
{
    if ( layer == 0 )
        this->removeSeries( _layer );
}
//=======================================================================================
void CustomScatter::clear_layers()
{
    this->removeSeries( _layer );
}
//=======================================================================================
void CustomScatter::clear_series()
{
    for ( auto& ser: this->seriesList() )
        this->removeSeries( ser );
}
//=======================================================================================
void CustomScatter::fill_layer( const QScatterDataArray& data, QScatter3DSeries& layer )
{
    layer.dataProxy()->removeItems( 0, layer.dataProxy()->itemCount() );
    layer.dataProxy()->addItems( data );
    layer.setItemSize( 0.05f );
    layer.setMesh( QAbstract3DSeries::MeshPoint );
}
//=======================================================================================

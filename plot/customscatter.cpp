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

    this->axisX()->setRange( - 20000, 20000 );
    this->axisY()->setRange( - 20000, 20000 );
    this->axisZ()->setRange( - 20000, 20000 );

    this->setShadowQuality( QAbstract3DGraph::ShadowQualityNone );

    //    auto preset = Q3DCamera::CameraPresetBehind;
    //    this->scene()->activeCamera()->setCameraPreset( ( Q3DCamera::CameraPreset ) preset );

    this->scene()->activeCamera()->setCameraPosition( - 2, 5, 250.0f );

    _layer = new QScatter3DSeries;

    addSeries( _layer );

    _layer->dataProxy()->addItems( _data );
    _layer->setBaseColor( qcolors.at( 1 ) );
    _layer->setItemSize( 0.05 );
    _layer->setMesh( QAbstract3DSeries::MeshPoint );

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
#include <iostream>
void CustomScatter::plot_pnts( const QList<LivoxRawPoint>& pnts,
                               const DrawPloperty& d_prop )
{
    if ( pnts.empty() )
        return;

    _data.clear();

    for ( const auto& pnt: pnts )
        _data << QVector3D( pnt.x, pnt.y, pnt.z );

    _layer->dataProxy()->addItems( _data );

    std::cout << "plot points" << std::endl;

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

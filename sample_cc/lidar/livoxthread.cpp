#include "livoxthread.h"

LivoxThread::LivoxThread( CustomScatter* scatter,
                          const std::vector<std::string>& broadcast )
    : _scatter   ( scatter   )
    , _broadcast ( broadcast )
{

}

void LivoxThread::run()
{
    LdsLidar& lidar = LdsLidar::GetInstance();

    lidar.scatter = _scatter;

    int ret = lidar.broadcast( _broadcast );

    if ( !ret )
        printf( "Init lds lidar success!\n" );

    else
        printf( "Init lds lidar fail!\n" );

    printf( "Start discovering device.\n" );

    DrawPloperty dprop;
    dprop.fn_name = "plot_pnts";
    dprop.need_replot = true;
    dprop.need_gr_clear = false;

    while ( true )
    {
        auto pnts = lidar.get_pnts();

        if ( pnts.size() == 10 )
            qt::vinvoke_queue( _scatter,
                               dprop.fn_name.c_str(),
                               Q_ARG( QList<LivoxRawPoint>, pnts ),
                               Q_ARG( DrawPloperty, dprop ) );
    }
}

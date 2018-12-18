#include "level.h"

MONSTER *new_monster( int life, int type, PHYSICS physics )
{
    static int nb = 0 ;

    MONSTER *monster = NULL ;
    Malloc( monster, MONSTER, 1 );
    __n_assert( monster, return NULL );

    memcpy( &monster -> physics, &physics, sizeof( PHYSICS ) );

    monster -> attr . life = life ;
    monster -> attr . type = type ;
    monster -> attr . action = ACTION_MOVE ;
    monster -> physics . sz = 15 ;
    nb ++ ;
    return monster;
}



int get_level_data( LEVEL *level, PHYSICS *physics, int mx, int my, int *x, int *y )
{
    int cellx = 0, celly = 0 ;

    cellx = ( physics -> position[ 0 ] + mx ) / level -> tilew ;
    celly = ( physics -> position[ 1 ] + my ) / level -> tileh ;

    if( cellx >= 0 && cellx < level -> w && celly >= 0 && celly < level -> h )
    {
        if( x != NULL )
            (*x) = cellx ;
        if( y != NULL )
            (*y) = celly ;

        return level -> cells[ cellx ][ celly ];
    }

    return -1000 ;
}



int test_coord( LEVEL *level, PHYSICS *physics, VECTOR3D fricton )
{
    int cellx = 0, celly = 0, done = 0, once = 0, has_blocked = 0 ;

    /* left */
    once = 0 ;
    do
    {
        int cellstate = get_level_data( level, physics, -physics -> sz, -physics -> sz, &cellx, &celly );

        if( cellstate != -1000 )
        {
            if( cellstate > 0 && cellstate < 4 )
            {
                physics -> position[ 0 ] = physics -> position[ 0 ] + 1.0 ;
                if( !once )
                {
                    once = has_blocked = 1 ;
                    physics -> speed[ 0 ] = abs( physics -> speed[ 0 ] ) - abs( fricton[ 0 ] );
                    if( physics -> speed[ 0 ] < 0.0 )
                        physics -> speed[ 0 ] = 0.0 ;
                }
                done = 0 ;
            }
            else
                done = 1 ;
        }
        else
            done = 1 ;
    }
    while( !done );

    /* right */
    once = 0 ;
    do
    {
        int cellstate = get_level_data( level, physics, +physics -> sz, -physics -> sz, &cellx, &celly );

        if( cellstate != -1000 )
        {
            if( cellstate > 0 && cellstate < 4 )
            {
                physics -> position[ 0 ] = physics -> position[ 0 ] - 1.0 ;
                if( !once )
                {
                    once = has_blocked = 1 ;
                    physics -> speed[ 0 ] = abs( fricton[ 0 ] ) - abs( physics -> speed[ 0 ] );
                    if( physics -> speed[ 0 ] > 0.0 )
                        physics -> speed[ 0 ] = 0.0 ;
                }
                done = 0 ;
            }
            else
                done = 1 ;
        }
        else
            done = 1 ;
    }
    while( !done );

    /* down */
    once = 0 ;
    do
    {
        int cellstate = get_level_data( level, physics, 0.0, 0.0, &cellx, &celly );

        if( cellstate != -1000 )
        {
            if( cellstate > 0 && cellstate < 4 )
            {
                physics -> position[ 1 ] = physics -> position[ 1 ] - 1.0 ;
                if( !once )
                {
                    once = has_blocked = 1 ;
                    physics -> speed[ 1 ] = abs( fricton[ 1 ] ) - abs( physics -> speed[ 1 ] );
                    if( physics -> speed[ 1 ] > 0.0 )
                        physics -> speed[ 1 ] = 0.0 ;
                    physics -> can_jump = 1 ;
                }
                done = 0 ;
            }
            else
                done = 1 ;
        }
        else
            done = 1 ;
    }
    while( !done );

    /* up */
    once = 0 ;
    do
    {
        int cellstate = get_level_data( level, physics, 0.0, -2.0 * physics -> sz, &cellx, &celly );

        if( cellstate != -1000 )
        {
            if( cellstate > 0 && cellstate < 4 )
            {
                physics -> position[ 1 ] = physics -> position[ 1 ] + 1.0 ;
                if( !once )
                {
                    once = has_blocked = 1 ;
                    physics -> speed[ 1 ] = abs( physics -> speed[ 1 ] ) - abs( fricton[ 1 ] ) ;
                    if( physics -> speed[ 1 ] < 0.0 )
                        physics -> speed[ 1 ] = 0.0 ;
                }
                done = 0 ;
            }
            else
                done = 1 ;
        }
        else
            done = 1 ;
    }
    while( !done );

    if( !has_blocked )
    {
        physics -> can_jump = 0 ;
    }

    return has_blocked ;
}



int animate_physics( LEVEL *level, PHYSICS *physics, VECTOR3D friction, double delta_t )
{
    VECTOR3D old_pos, new_pos, pos_delta ;

    memcpy( &old_pos, &physics -> position, sizeof( VECTOR3D ) );

    update_physics_position( physics, delta_t );

    memcpy( &new_pos, &physics -> position, sizeof( VECTOR3D ) );

    double d = distance ( old_pos, physics -> position );
    double steps = physics -> sz ;

    if( d >= steps )
    {
        for( int it = 0 ; it < 3 ; it ++ )
            pos_delta[ it ] = ( physics -> position[ it ] - old_pos[ it ] ) / d ;
        double inc = -steps ;

        while( inc < d )
        {
            inc += steps ;
            if( inc > d )
                inc = d ;
            for( int it = 0 ; it < 3 ; it ++ )
                physics -> position[ it ] = old_pos[ it ] + pos_delta[ it ] * inc ;

            if( test_coord( level, physics, friction ) > 0 )
                break ;
        }
    }
    else
        test_coord( level, physics, friction );

    return TRUE ;
}



int animate_monster( LEVEL *level, double delta_t )
{
    VECTOR3D friction = { 0.0, 500.0, 0.0 };
    LIST_NODE *node = NULL ;
    MONSTER *monster = NULL ;
    __n_assert( level, return FALSE );
    __n_assert( level -> monster_list, return FALSE );

    node = level -> monster_list -> start ;
    while( node )
    {
        monster= (MONSTER *)node -> ptr ;
        if( monster -> attr . life <= 0 )
        {
            LIST_NODE *ntokill = node ;
            if( node -> next )
                node = node -> next ;
            else
                node = NULL ;
            MONSTER *mtokill = remove_list_node( level -> monster_list, ntokill, MONSTER );
            Free( mtokill );
        }
        else
        {
            monster -> physics . can_jump = 0 ;
            animate_physics( level, &monster -> physics, friction, delta_t );

            int cellx = 0, celly = 0, inc = 0 ;
            int cellstate = get_level_data( level, &monster -> physics, 0.0, 0.0, &cellx, &celly );

            if( cellstate == 1 )
                monster -> attr . life = 0 ;

            if( monster -> physics . speed[ 0 ] > 0 )
            {
                inc = 1 ;
            }
            else
                inc = -1 ;

            if( cellstate != -1 )
            {
                if( ( ( cellx + inc ) >= 0 ) && ( ( cellx + inc ) < level -> w ) && ( ( celly - 1 ) > 0 ) && ( ( celly + 1 ) < level -> h ) )
                {
                    if( monster -> physics . can_jump == 1  )
                    {
                        if( monster -> attr . type != PASSIVE_MONSTER )
                        {
                            /* jump if next block is reachable */
                            if( level -> cells[ cellx + inc ][ celly ] > 0 && level -> cells[ cellx + inc ][ celly - 1 ] == 0 )
                            {
                                monster -> physics . speed[ 1 ] = -400.0 ;
                            }
                            /* jump if there is a gap thin enough */
                            if( level -> cells[ cellx + inc ][ celly + 1 ] == 0 && level -> cells[ cellx + inc ][ celly ] == 0 )
                            {
                                monster -> physics . speed[ 1 ] = -400.0 ;
                            }
                        }
                        else
                        {
                            /* turn over if next block is empty */
                            if( level -> cells[ cellx + inc ][ celly + 1 ] == 0 )
                            {
                                if( inc > 0 )
                                {
                                    monster -> physics . speed[ 0 ] = -abs( monster -> physics . speed[ 0 ] );
                                }
                                if( inc < 0 )
                                {
                                    monster -> physics . speed[ 0 ] = abs( monster -> physics . speed[ 0 ] );
                                }
                            }
                        }
                    }
                }
            }
            node = node -> next ;
        }
    }
    return TRUE ;
}



LEVEL *load_level( char *file, char *resfile, int w, int h )
{
    FILE *in = NULL ;
    LEVEL *lvl = NULL ;

    MONSTER *monster = NULL ;

    in = fopen( file, "r" );
    if( !in )
    {
        n_log( LOG_ERR, "%s not found", file );
        return NULL ;
    }

    Malloc( lvl, LEVEL, 1 );
    __n_assert( lvl, return NULL );

    char *str = NULL ;
    Malloc( str, char, 1024 );
    __n_assert( str, return NULL );

    int m1 = 0, m2 = 0, m3 = 0 ;
    fgets( str, 1024, in );
    sscanf( str, "%d %d %d %d %d %d %d %d", &lvl -> w, &lvl -> h, &m1, &m2, &m3, &lvl -> tilew, &lvl -> tileh, &lvl -> nb_to_rescue );

    Malloc( lvl -> rescue_icons, ALLEGRO_BITMAP *, lvl -> nb_to_rescue  );

    for( int it = 0 ; it < 3 ; it ++ )
    {
        char tmpstr[ 1024 ] = "" ;
        fscanf( in, "%s", tmpstr );
        n_log( LOG_DEBUG, "loading monster bmp %s", tmpstr );
        lvl -> monsters[ it ] = al_load_bitmap( tmpstr );
    }
    for( int it = 0 ; it < 5 ; it ++ )
    {
        char tmpstr[ 1024 ] = "" ;
        fscanf( in, "%s", tmpstr );
        n_log( LOG_DEBUG, "loading tile bmp %s", tmpstr );
        lvl -> tiles[ it ] = al_load_bitmap( tmpstr );
    }

    for( int it = 0 ; it < lvl -> nb_to_rescue ; it ++ )
    {
        char tmpstr[ 1024 ] = "" ;
        fscanf( in, "%s", tmpstr );
        n_log( LOG_DEBUG, "loading rescue icon bmp %s", tmpstr );
        lvl -> rescue_icons[ it ] = al_load_bitmap( tmpstr );
    }

    n_log( LOG_DEBUG, "Level parameters: %d %d %d %d %d %d %d", lvl -> w, lvl -> h, m1, m2, m3, lvl -> tilew, lvl -> tileh );

    n_log( LOG_DEBUG, "Loading ressources from config %s", resfile );

    lvl -> cells = (int **)calloc( lvl -> w, sizeof( void * ) );
    for( int x = 0 ; x < lvl -> w ; x ++ )
    {
        lvl -> cells[ x ] = (int *)calloc( lvl -> h, sizeof( void * ) );
    }
    for( int x = 0 ; x < lvl -> w ; x ++ )
        for( int y = 0 ;  y < lvl -> h ; y ++ )
            lvl -> cells[ x ][ y ] = 0 ;

    lvl -> monster_list = new_generic_list( -1 );

    int y = 0 ;
    while( y < lvl -> h && fgets( str, 1024, in ) )
    {
        for( int x = 0 ; (unsigned)x < strlen( str ) && x < lvl -> w ; x ++ )
        {
            lvl -> cells[ x ][ y ] = 0 ;
            if( str[ x ] == '#' )
                lvl -> cells[ x ][ y ] = 1 ;
            else if( str[ x ] == '*' )
                lvl -> cells[ x ][ y ] = 2 ;
            else if( str[ x ] == 'B' )
                lvl -> cells[ x ][ y ] = 11 ;
            else if( str[ x ] == 'N' )
                lvl -> cells[ x ][ y ] = 3 ;
            else if( str[ x ] == 'M' )
            {
                PHYSICS mpos ;
                mpos . can_jump = 1 ;
                double vx = 0.0 ;
                while( abs( vx ) < 75.0 )
                    vx = 200.0 - rand()%400 ;
                VECTOR3D_SET( mpos . position, ( x * lvl -> tilew ) + lvl -> tilew / 2.0, y * lvl -> tileh - lvl -> tileh / 2.0, 0.0 );
                VECTOR3D_SET( mpos . speed, vx, 0.0, 0.0 );
                VECTOR3D_SET( mpos . acceleration, 0.0, 500.0, 0.0 );
                VECTOR3D_SET( mpos . orientation, 0.0, 0.0, 0.0 );
                monster = new_monster( 50, PASSIVE_MONSTER, mpos );
                if( monster )
                    list_push( lvl -> monster_list, monster, NULL );
            }
            else if( str[ x ] == 'S' )
            {
                PHYSICS mpos ;
                mpos . can_jump = 1 ;
                double vx = 0.0 ;
                while( abs( vx ) < 175.0 )
                    vx = 200.0 - rand()%400 ;
                VECTOR3D_SET( mpos . position, ( x * lvl -> tilew ) + lvl -> tilew / 2.0, y * lvl -> tileh - lvl -> tileh / 2.0, 0.0 );
                VECTOR3D_SET( mpos . speed, vx, 0.0, 0.0 );
                VECTOR3D_SET( mpos . acceleration, 0.0, 500.0, 0.0 );
                VECTOR3D_SET( mpos . orientation, 0.0, 0.0, 0.0 );
                monster = new_monster( 100, AGRESSIVE_MONSTER, mpos );
                if( monster )
                    list_push( lvl -> monster_list, monster, NULL );
            }
            else if( str[ x ] == 'G' )
            {
                PHYSICS mpos ;
                mpos . can_jump = 1 ;
                double vx = 0.0 ;
                while( abs( vx ) < 175.0 )
                    vx = 200.0 - rand()%400 ;
                VECTOR3D_SET( mpos . position, ( x * lvl -> tilew ) + lvl -> tilew / 2.0, y * lvl -> tileh - lvl -> tileh / 2.0, 0.0 );
                VECTOR3D_SET( mpos . speed, vx, 0.0, 0.0 );
                VECTOR3D_SET( mpos . acceleration, 0.0, 500.0, 0.0 );
                VECTOR3D_SET( mpos . orientation, 0.0, 0.0, 0.0 );
                monster = new_monster( 150, BIGBOSS_MONSTER, mpos );
                if( monster )
                    list_push( lvl -> monster_list, monster, NULL );
            }
            else if( str[ x ] == 'P' )
            {
                lvl -> startx = lvl -> tilew * x ;
                lvl -> starty = lvl -> tileh * y ;
            }
            else if( str[ x ] == '@' )
            {
                lvl -> cells[ x ][ y ] = -2 ;
            }
            else
            {
                int id = 0 ;
                if( str[ x ] == '1' || str[ x ] == '2' || str[ x ] == '3'|| str[ x ] == '4'||  str[ x ] == '5'||  str[ x ] == '6'||  str[ x ] == '7'||  str[ x ] == '8'||  str[ x ] == '9'  )
                {
                    if( str[ x+1] == ' ' )
                    {
                        if( str_to_int_ex( &str[x], 0, 1, &id, 10 ) == TRUE )
                        {
                            id += 100 ;
                            lvl -> cells[ x ][ y ] = id ;
                        }
                    }
                    else
                    {
                        if( str_to_int_ex( &str[x], 0, 2, &id, 10 ) == TRUE )
                        {
                            id += 100 ;
                            lvl -> cells[ x ][ y ] = id ;
                            x++;
                        }
                    }

                }
            }
        }
        y++ ;
    }
    lvl -> native_w = w ;
    lvl -> native_h = h ;

    n_log( LOG_INFO, "Level %s loaded", file );

    return lvl ;
}



int draw_level( LEVEL *lvl, int px, int py, int w, int h )
{
    int startx =  ( px - w / 2 ) / lvl -> tilew ;
    int starty =  ( py - h / 2 ) / lvl -> tileh ;
    int endx = 1 + w / lvl -> tilew ;
    int endy = 1 + h / lvl -> tileh ;

    int mx = 0, my = 0 ;

    mx = ( px - w / 2 ) - lvl -> tilew * startx ;
    my = ( py - h / 2 ) - lvl -> tileh * starty ;


    double xmin = 0.0, ymin = 0.0, xmax = 0.0, ymax = 0.0 ;
    for( int x = 0 ; x <= endx ; x ++ )
    {
        for( int y = 0 ; y <= endy ; y ++ )
        {
            int x1 = x * lvl -> tilew - mx ;
            int y1 = y * lvl -> tileh - my ;

            int x2 = x1 + lvl -> tilew ;
            int y2 = y1 + lvl -> tileh ;

            if( x1 < xmin )
                xmin = x1 ;
            if( x2 < xmin )
                xmin = x2 ;
            if( y1 < ymin )
                ymin = y1 ;
            if( y2 < ymin )
                ymin = y2 ;
            if( x1 > xmax )
                xmax = x1 ;
            if( x2 > xmax )
                xmax = x2 ;
            if( y1 > ymax )
                ymax = y1 ;
            if( y2 > ymax )
                ymax = y2 ;


            int px = x + startx ;
            int py = y + starty ;

            if( px >= 0 && py >= 0 && px < lvl -> w && py < lvl -> h )
            {
                if( lvl -> cells[ px ][ py ] > 0 && lvl -> cells[ px ][ py ] < 4 )
                {

                    ALLEGRO_BITMAP *bmp = lvl -> tiles[ lvl -> cells[ px ][ py ] - 1 ] ;
                    if( bmp )
                    {
                        al_draw_bitmap( bmp, x1, y1, 0 );
                    }
                    else
                    {
                        if( lvl -> cells[ px ][ py ] == 1 )
                            al_draw_rectangle( x1, y1, x2, y2, al_map_rgb( 255, 255, 255 ), 1 );
                        if( lvl -> cells[ px ][ py ] == 2 )
                            al_draw_rectangle( x1, y1, x2, y2, al_map_rgb( 155, 155, 155 ), 1 );
                        if( lvl -> cells[ px ][ py ] == 3 )
                            al_draw_rectangle( x1, y1, x2, y2, al_map_rgb( 155, 55, 55 ), 1 );
                    }
                }
                else if(  lvl -> cells[ px ][ py ] >= 100 && lvl -> cells[ px ][ py ] <= (100 + lvl -> nb_to_rescue ) )
                {
                    ALLEGRO_BITMAP *bmp = lvl -> rescue_icons[ lvl -> cells[ px ][ py ] - 101 ] ;
                    if( bmp )
                    {
                        int w = al_get_bitmap_width( bmp );
                        int h = al_get_bitmap_height( bmp );

                        al_draw_bitmap( bmp, x1 + (lvl -> tilew / 2) - w / 2, y1 - h + lvl -> tileh, 0 );
                    }
                    else
                    {
                        al_draw_rectangle( x1, y1, x2, y2, al_map_rgb( 255, 255, 255 ), 1 );
                    }
                }
                else if( lvl -> cells[ px ][ py ] == -2 )
                {
                    ALLEGRO_BITMAP *bmp = lvl -> tiles[ 4 ];
                    if( bmp )
                    {
                        int w = al_get_bitmap_width( bmp );
                        int h = al_get_bitmap_height( bmp );

                        al_draw_bitmap( bmp, x1 + (lvl -> tilew / 2) - w / 2, y1 - h + lvl -> tileh, 0 );
                    }
                    else
                    {
                        al_draw_filled_rectangle( x1, y1, x2, y2, al_map_rgb( 255, 255, 255 ) );
                    }
                }
                else if( lvl -> cells[ px ][ py ] == 11 )
                {
                    ALLEGRO_BITMAP *bmp = lvl -> tiles[ 3 ];
                    if( bmp )
                    {
                        int w = al_get_bitmap_width( bmp );
                        int h = al_get_bitmap_height( bmp );

                        al_draw_bitmap( bmp, x1 + (lvl -> tilew / 2) - w / 2, y1 - h + lvl -> tileh, 0 );
                    }
                    else
                    {
                        al_draw_filled_rectangle( x1, y1, x2, y2, al_map_rgb( 255, 255, 255 ) );
                    }
                }

            }
        }
    }

    LIST_NODE *node = NULL ;
    node = lvl -> monster_list -> start ;
    int it = 0 ;
    while( node )
    {
        MONSTER *monster = node -> ptr ;
        if( monster )
        {
            double x = w / 2 + monster -> physics . position[ 0 ] - px - monster -> physics .sz ;
            double y = h / 2 + monster -> physics . position[ 1 ] - py ;
            ALLEGRO_BITMAP *bmp = lvl -> monsters[ monster -> attr . type ] ;
            if( bmp )
            {
                int bmpw = al_get_bitmap_width( bmp );
                int bmph = al_get_bitmap_height( bmp );
                al_draw_bitmap( bmp, x - bmpw / 2, y - bmph, 0 );
            }
            else
            {
                al_draw_rectangle( x, y, x + 2 * monster -> physics . sz, y - 2 * monster -> physics . sz, al_map_rgb( 255, 0, 0 ), 1 );
                /*textprintf_ex( bmp , font , 5 , 55 + it * 10 , makecol( 255,255,255) , 1 , "M%d %g %g" , it , x , y );*/
            }
        }
        node = node -> next ;
        it ++ ;
    }
    return TRUE ;
}

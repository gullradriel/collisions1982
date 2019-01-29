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
    monster -> attr . xp = 0 ;
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



int test_coord( LEVEL *level, PHYSICS *physics, VECTOR3D fricton, int off_x, int off_y )
{
    int cellx = 0, celly = 0, done = 0, once = 0, has_blocked = 0 ;

    /* left */
    once = 0 ;
    do
    {
        int cellstate = get_level_data( level, physics, off_x - physics -> sz, off_y, &cellx, &celly );

        if( cellstate != -1000 )
        {
            if( cellstate > 0 && cellstate < 4 )
            {
                physics -> position[ 0 ] = physics -> position[ 0 ] + 1.0 ;
                if( !once )
                {
                    once = has_blocked = 1 ;
                    physics -> speed[ 0 ] = abs(  abs(physics -> speed[ 0 ]) -  abs(fricton[ 0 ]) );
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
        int cellstate = get_level_data( level, physics, off_x + physics -> sz, off_y, &cellx, &celly );

        if( cellstate != -1000 )
        {
            if( cellstate > 0 && cellstate < 4 )
            {
                physics -> position[ 0 ] = physics -> position[ 0 ] - 1.0 ;
                if( !once )
                {
                    once = has_blocked = 1 ;
                    physics -> speed[ 0 ] = - abs(  abs(physics -> speed[ 0 ]) -  abs(fricton[ 0 ]) );
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
        int cellstate = get_level_data( level, physics, off_x, off_y + physics -> sz, &cellx, &celly );

        if( cellstate != -1000 )
        {
            if( cellstate > 0 && cellstate < 4 )
            {
                physics -> position[ 1 ] = physics -> position[ 1 ] - 1.0 ;
                if( !once )
                {
                    once = has_blocked = 1 ;
                    physics -> speed[ 1 ] = -abs( abs( physics -> speed[ 1 ] ) - abs(fricton[ 1 ] ) );
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
        int cellstate = get_level_data( level, physics,  off_x, off_y - physics -> sz, &cellx, &celly );

        if( cellstate != -1000 )
        {
            if( cellstate > 0 && cellstate < 4 )
            {
                physics -> position[ 1 ] = physics -> position[ 1 ] + 1.0 ;
                if( !once )
                {
                    once = has_blocked = 1 ;
                    physics -> speed[ 1 ] = abs( abs(physics -> speed[ 1 ] ) - abs( fricton[ 1 ] ) );
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

    if( physics -> sz < 1 )
        physics -> sz = 1 ;

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

            if( test_coord( level, physics, friction, 0, 0 ) > 0 )
                break ;
        }
    }
    else
        test_coord( level, physics, friction, 0, 0 );

    return TRUE ;
}

int animate_level( LEVEL *level,  PLAYER *player, double delta_t )
{
    VECTOR3D friction = { 0.0, 500.0, 0.0 };
    LIST_NODE *node = NULL ;
    MONSTER *monster = NULL ;
    __n_assert( level, return FALSE );
    __n_assert( level -> monster_list, return FALSE );

    PARTICLE *ptr = NULL ;

    node = level -> particle_system_effects -> list -> start ;
    while( node )
    {
        ptr = (PARTICLE *)node -> ptr ;
        if( ptr -> lifetime != -1 )
        {
            ptr -> lifetime -= delta_t/1000.0 ;
            if( ptr -> lifetime == -1 )
                ptr -> lifetime = 0 ;
        }

        if( ptr -> lifetime > 0 || ptr -> lifetime == -1 )
        {
            animate_physics( level, &ptr -> object, friction, delta_t );
            node = node -> next ;
        }
        else
        {
            LIST_NODE *node_to_kill = node ;
            node = node -> next ;
            ptr = remove_list_node( level -> particle_system_effects -> list, node_to_kill, PARTICLE );
            Free( ptr );
        }
    }
    LIST_NODE *bnode = level -> particle_system_bullets -> list -> start ;
    while( bnode )
    {
        PARTICLE *bptr = (PARTICLE *)bnode -> ptr ;
        if( bptr )
        {
            if( bptr -> lifetime != -1 )
            {
                bptr -> lifetime -= delta_t/1000.0 ;
                if( bptr -> lifetime == -1 )
                    bptr -> lifetime = 0 ;
            }

            if( bptr -> lifetime > 0 || bptr -> lifetime == -1 )
            {
                node = level -> monster_list -> start ;
                while( node )
                {
                    monster= (MONSTER *)node -> ptr ;
                    /*x, y, x + 2 * monster -> physics . sz, y - 2 * monster -> physics . sz,*/
                    if(     bptr -> object . position[ 0 ] > monster -> physics . position[ 0 ] &&
                            bptr -> object . position[ 1 ] < monster -> physics . position[ 1 ] &&
                            bptr -> object . position[ 0 ] < monster -> physics . position[ 0 ] + 2 * monster ->physics . sz &&
                            bptr -> object . position[ 1 ] > monster -> physics . position[ 1 ] - 2 * monster ->physics . sz  )
                    {
                        bptr -> lifetime = 0 ;
                        monster -> attr . life = 0 ;

                        (*player) . attr . xp += 250 ;
                        if( (*player) . attr . xp > (*player) . attr . xp_to_level )
                        {
                            if( (*player) . attr . level < 10 )
                            {
                                (*player) . attr . level ++ ;
                                (*player) . attr . xp_to_level += 2000 ;
                            }
                        }

                        PHYSICS tmp_part ;
                        tmp_part . sz = 10 ;
                        for( int it = 0 ; it < 30 ; it ++ )
                        {
                            VECTOR3D_SET( tmp_part . speed,
                                          300 - rand()%600,
                                          300 - rand()%600,
                                          0.0  );
                            VECTOR3D_SET( tmp_part . position, monster -> physics . position[ 0 ], monster ->  physics . position[ 1 ], 0.0  );
                            VECTOR3D_SET( tmp_part . acceleration, 0.0, 0.0, 0.0 );
                            VECTOR3D_SET( tmp_part . orientation, 0.0, 0.0, 0.0 );
                            add_particle( level -> particle_system_effects, -1, PIXEL_PART, 100 + rand()%500, 1+rand()%3,
                                          al_map_rgba(   55 + rand()%200,  0, 0, 100 + rand()%155 ), tmp_part );

                        }
                    }
                    node = node -> next ;
                }
                animate_physics( level, &bptr -> object, friction, delta_t );
                bnode = bnode -> next ;
            }
            else
            {
                LIST_NODE *node_to_kill = bnode ;
                bnode = bnode -> next ;
                ptr = remove_list_node( level -> particle_system_bullets -> list, node_to_kill, PARTICLE );
                Free( ptr );
            }
        }
    }


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

            int cellx = 0, celly = 0 ;
            int cellstate = get_level_data( level, &monster -> physics, 0.0, 0.0, &cellx, &celly );

            if( cellstate == 1 )
                monster -> attr . life = 0 ;

            double itx = 0, ity = 0, itz = 0, sz = 0;

            itx = monster -> physics . position[ 0 ] - (*player) . physics . position[ 0 ] ;
            ity = monster -> physics . position[ 1 ] - (*player) . physics . position[ 1 ] ;
            itz = 0 ;

            if( abs( itx < level -> native_w / 2 ) )
            {
                sz = sqrt((itx * itx) + (ity * ity) + (itz * itz));
                itx /= sz ;
                ity /= sz ;
                itz /= sz ;
                itx*= 100 ;
                ity*= 100 ;
                itz*= 100 ;
                VECTOR3D_SET( monster -> physics . speed, -itx, -ity, -itz );
                VECTOR3D_SET( monster -> physics . acceleration, -itx, -ity, -itz );
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
    sscanf( str, "%d %d %d %d %d %d %d", &lvl -> w, &lvl -> h, &m1, &m2, &m3, &lvl -> tilew, &lvl -> tileh );

    for( int it = 0 ; it < 4 ; it ++ )
    {
        char tmpstr[ 1024 ] = "" ;
        fscanf( in, "%s", tmpstr );
        n_log( LOG_DEBUG, "loading tile bmp %s", tmpstr );
        lvl -> tiles[ it ] = al_load_bitmap( tmpstr );
    }
    n_log( LOG_DEBUG, "Level parameters: %d %d %d %d %d %d %d", lvl -> w, lvl -> h, m1, m2, m3, lvl -> tilew, lvl -> tileh );

    n_log( LOG_DEBUG, "Loading ressources from config %s", resfile );

    init_particle_system( &lvl -> particle_system_bullets, 5000, 0, 0, 0, 100 );

    init_particle_system( &lvl -> particle_system_effects, 5000, 0, 0, 0, 100 );


    lvl -> cells = (int **)calloc( lvl -> w, sizeof( void * ) );
    for( int x = 0 ; x < lvl -> w ; x ++ )
    {
        lvl -> cells[ x ] = (int *)calloc( lvl -> h, sizeof( void * ) );
    }
    for( int x = 0 ; x < lvl -> w ; x ++ )
        for( int y = 0 ;  y < lvl -> h ; y ++ )
            lvl -> cells[ x ][ y ] = 1 ;


    for( int y = 1 ;  y < lvl -> h - 1 ; y ++ )
        lvl -> cells[ 1 ][ y ] = 0 ;

    lvl -> monster_list = new_generic_list( -1 );

    /* starting pos */
    lvl -> startx = lvl -> tilew * 2 ;
    lvl -> starty = lvl -> tileh * 10 ;

    int nb_crawlers = lvl -> h / 5 ;
    int crawlers[ nb_crawlers ] ;
    for( int it = 0 ; it < nb_crawlers ; it ++ )
    {
        crawlers[ it ] = 1 + rand()%(lvl -> h - 3);
    }
    /* Now draw a level */
    for( int x = 1 ; x < lvl -> w ; x ++ )
    {
        for( int it = 0 ; it < nb_crawlers ; it ++ )
        {
            crawlers[ it ] = crawlers[ it ] + 2 - rand()%5 ;
            if( crawlers[ it ] < 2 )
                crawlers[ it ] = 2 ;
            if( crawlers[ it ] > lvl -> h -3 )
                crawlers[ it ] = lvl -> h -3 ;
            for( int x_it = x - 2 ; x_it < x + 2 ; x_it ++ )
            {
                for( int y_it = crawlers[ it ] - 2 ; y_it < crawlers[ it ] + 2 ; y_it ++ )
                {
                    if( x_it > 1 && x_it < lvl -> w &&
                            y_it > 1 && y_it < lvl -> h - 2 )
                        lvl -> cells[ x_it ][ y_it] = 0 ;
                }
            }
        }
    }
    for( int x = lvl -> w -2 ; x <= lvl -> w - 2 ; x ++ )
    {
        for( int y = 1 ; y < lvl -> h - 1 ; y ++ )
        {
            lvl -> cells[ x ][ y ] = -2 ;
        }
    }
    for( int it = 0 ; it < 800 ; it ++ )
    {
        int x = 20 + rand()%( lvl ->w -20 );
        int y = 1 + rand()%( lvl -> h - 2 );
        while( lvl -> cells[ x][ y] != 0 )
        {
            x = 20 + rand()%( lvl ->w -20 );
            y = 1 + rand()%( lvl -> h - 2 );
        }
        PHYSICS mpos ;
        mpos . can_jump = 1 ;
        double vx = 0.0 ;
        VECTOR3D_SET( mpos . position, ( x * lvl -> tilew ) + lvl -> tilew / 2.0, y * lvl -> tileh - lvl -> tileh / 2.0, 0.0 );
        VECTOR3D_SET( mpos . speed, vx, 0.0, 0.0 );
        VECTOR3D_SET( mpos . acceleration, 0.0, 0.0, 0.0 );
        VECTOR3D_SET( mpos . orientation, 0.0, 0.0, 0.0 );
        monster = new_monster( 100, AGRESSIVE_MONSTER, mpos );
        if( monster )
            list_push( lvl -> monster_list, monster, NULL );
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

                    /*ALLEGRO_BITMAP *bmp = lvl -> tiles[ lvl -> cells[ px ][ py ] - 1 ] ;
                    if( bmp )
                    {
                        al_draw_bitmap( bmp, x1, y1, 0 );
                    }
                    else
                    {*/
                    if( lvl -> cells[ px ][ py ] == 1 )
                        al_draw_rectangle( x1, y1, x2, y2, al_map_rgb( 255, 255, 255 ), 2 );
                    if( lvl -> cells[ px ][ py ] == 2 )
                        al_draw_rectangle( x1, y1, x2, y2, al_map_rgb( 155, 155, 155 ), 2 );
                    if( lvl -> cells[ px ][ py ] == 3 )
                        al_draw_rectangle( x1, y1, x2, y2, al_map_rgb( 155, 55, 55 ), 2 );
                    /*}*/
                }
                else if( lvl -> cells[ px ][ py ] == -2 )
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
            al_draw_rectangle( x, y, x + 2 * monster -> physics . sz, y - 2 * monster -> physics . sz, al_map_rgb( 255, 0, 0 ), 1 );
        }
        node = node -> next ;
        it ++ ;
    }

    draw_particle( lvl -> particle_system_effects, px - w / 2, py - h / 2, w, h, 50 );
    draw_particle( lvl -> particle_system_bullets, px - w / 2, py - h / 2, w, h, 50 );

    return TRUE ;
}

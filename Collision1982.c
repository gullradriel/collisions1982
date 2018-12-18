/**\file Collision1982.c
 *
 *  Collision Main File
 *
 *\author Castagnier Mickaël
 *
 *\version 1.0
 *
 *\date 17/12/2018
 *
Gift asked:
1. bouncing balls.
2. multiplayer (optional)
3. pixel style.
 */

#define WIDTH 1280
#define HEIGHT 800

#include "nilorea/n_common.h"
#include "nilorea/particle.h"
#include "nilorea/n_anim.h"
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_ttf.h>
#include "level.h"

ALLEGRO_DISPLAY *display  = NULL ;

/******************************************************************************
 *                           VARIOUS DECLARATIONS                             *
 ******************************************************************************/

int		DONE = 0,                    /* Flag to check if we are always running */
        getoptret = 0,				  /* launch parameter check */
        log_level = LOG_ERR ;			 /* default LOG_LEVEL */

ALLEGRO_BITMAP *scr_buf       = NULL ;

ALLEGRO_TIMER *fps_timer = NULL ;
ALLEGRO_TIMER *logic_timer = NULL ;
LIST *active_object = NULL ;                      /* list of active objects */

LEVEL *level = NULL ;
PARTICLE_SYSTEM *particle_system = NULL ;

int main( int argc, char *argv[] )
{
    /*
     * INITIALISATION
     */
    set_log_level( LOG_NOTICE );

    N_STR *log_file = NULL ;
    nstrprintf( log_file, "%s.log", argv[ 0 ] ) ;
    /*set_log_file( _nstr( log_file ) );*/
    free_nstr( &log_file );

    n_log( LOG_NOTICE, "%s is starting ...", argv[ 0 ] );

    /* allegro 5 + addons loading */
    if (!al_init())
    {
        n_abort("Could not init Allegro.\n");
    }
    if (!al_install_audio())
    {
        n_abort("Unable to initialize audio addon\n");
    }
    if (!al_init_acodec_addon())
    {
        n_abort("Unable to initialize acoded addon\n");
    }
    if (!al_init_image_addon())
    {
        n_abort("Unable to initialize image addon\n");
    }
    if (!al_init_primitives_addon() )
    {
        n_abort("Unable to initialize primitives addon\n");
    }
    if( !al_init_font_addon() )
    {
        n_abort("Unable to initialize font addon\n");
    }
    if( !al_init_ttf_addon() )
    {
        n_abort("Unable to initialize ttf_font addon\n");
    }
    if( !al_install_keyboard() )
    {
        n_abort("Unable to initialize keyboard handler\n");
    }
    if( !al_install_mouse())
    {
        n_abort("Unable to initialize mouse handler\n");
    }
    ALLEGRO_EVENT_QUEUE *event_queue = NULL;

    event_queue = al_create_event_queue();
    if(!event_queue)
    {
        fprintf(stderr, "failed to create event_queue!\n");
        al_destroy_display(display);
        return -1;
    }
    char ver_str[ 128 ] = "" ;

    while( ( getoptret = getopt( argc, argv, "hvV:L:" ) ) != EOF )
    {
        switch( getoptret )
        {
        case 'h':
            n_log( LOG_NOTICE, "\n    %s -h help -v version -V DEBUGLEVEL (NOLOG,VERBOSE,NOTICE,ERROR,DEBUG)\n", argv[ 0 ] );
            exit( TRUE );
        case 'v':
            sprintf( ver_str, "%s %s", __DATE__, __TIME__ );
            exit( TRUE );
            break ;
        case 'V':
            if( !strncmp( "NOTICE", optarg, 6 ) )
            {
                log_level = LOG_INFO ;
            }
            else
            {
                if(  !strncmp( "VERBOSE", optarg, 7 ) )
                {
                    log_level = LOG_NOTICE ;
                }
                else
                {
                    if(  !strncmp( "ERROR", optarg, 5 ) )
                    {
                        log_level = LOG_ERR ;
                    }
                    else
                    {
                        if(  !strncmp( "DEBUG", optarg, 5 ) )
                        {
                            log_level = LOG_DEBUG ;
                        }
                        else
                        {
                            n_log( LOG_ERR, "%s is not a valid log level\n", optarg );
                            exit( FALSE );
                        }
                    }
                }
            }
            n_log( LOG_NOTICE, "LOG LEVEL UP TO: %d\n", log_level );
            set_log_level( log_level );
            break;
        case 'L' :
            n_log( LOG_NOTICE, "LOG FILE: %s\n", optarg );
            set_log_file( optarg );
            break ;
        case '?' :
        {
            switch( optopt )
            {
            case 'V' :
                n_log( LOG_ERR, "\nPlease specify a log level after -V. \nAvailable values: NOLOG,VERBOSE,NOTICE,ERROR,DEBUG\n\n" );
                break;
            case 'L' :
                n_log( LOG_ERR, "\nPlease specify a log file after -L\n" );
            default:
                break;
            }
        }
        default:
            n_log( LOG_ERR, "\n    %s -h help -v version -V DEBUGLEVEL (NOLOG,VERBOSE,NOTICE,ERROR,DEBUG) -L logfile\n", argv[ 0 ] );
            exit( FALSE );
        }
    }

    fps_timer = al_create_timer( 1.0/30.0 );
    logic_timer = al_create_timer( 1.0/50.0 );

    al_set_new_display_flags( ALLEGRO_OPENGL|ALLEGRO_WINDOWED );
    display = al_create_display( WIDTH, HEIGHT );
    if( !display )
    {
        n_abort("Unable to create display\n");
    }
    al_set_window_title( display, "SantaToTheRescue [Extended]" );
    al_set_new_bitmap_flags( ALLEGRO_VIDEO_BITMAP );

    ALLEGRO_FONT *font = al_load_font( "DATA/2Dumb.ttf", 18, 0 );

    init_particle_system( &particle_system, 5000, 0, 0, 0, 100 );

    DONE = 0 ;

    double delta_time = 1000000.0/40.0 ;

    active_object = new_generic_list( -1 );

    PLAYER player ;

    LEVEL *level = NULL ;
    level = load_level( "DATA/Levels/level1.txt", "DATA/Levels/level1_ressources.txt", WIDTH, HEIGHT );
    int nb_to_rescue = level -> nb_to_rescue ;
    int used_magic_blocks = 0, max_magic_blocks = 0 ;

    VECTOR3D_SET( player . physics . position, level -> startx, level -> starty, 0.0 );
    VECTOR3D_SET( player . physics . speed,  0.0, 0.0, 0.0  );
    VECTOR3D_SET( player . physics . acceleration, 0.0, 300.0, 0.0 );
    VECTOR3D_SET( player . physics . orientation, 0.0, 0.0, 0.0 );

    player . physics . can_jump = 0, player . physics . sz = 3 ;
    player . attr . action = 0 ;
    player . attr . move = 0 ;
    player . attr . life = 100 ;
    player . attr . direction = 0 ;
    player . attr . xspeedmax = 200 ;
    player . attr . xspeedinc = 30 ;
    player . attr . yspeedmax = 500 ;
    player . attr . yspeedinc = player . attr . yspeedmax ;
    enum APP_KEYS
    {
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ESC, KEY_SPACE, KEY_CTRL
    };
    int key[ 7 ] = {false,false,false,false,false,false,false};
    VECTOR3D old_pos ;
    VECTOR3D friction = { 800.0, 800.0, 0.0 };

    al_register_event_source(event_queue, al_get_display_event_source(display));

    al_start_timer( fps_timer );
    al_start_timer( logic_timer );
    al_register_event_source(event_queue, al_get_timer_event_source(fps_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(logic_timer));

    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

    ALLEGRO_BITMAP *scrbuf = al_create_bitmap( WIDTH, HEIGHT );

    al_hide_mouse_cursor(display);

    int mx = 0, my = 0, mouse_b1 = 0, mouse_b2 = 0 ;
    int do_draw = 0, do_logic = 0 ;

    /* ANIM_LIB *anim_lib = create_anim_library( "player" , 3 );
       add_bmp_to_lib( anim_lib , 0 , "DATA/Gfxs/golem_gauche.bmp" , "DATA/Gfxs/golem_gauche.txt" );
       add_bmp_to_lib( anim_lib , 1 , "DATA/Gfxs/golem_droite.bmp" , "DATA/Gfxs/golem_droite.txt" ); */
    ALLEGRO_BITMAP *player_gfx[ 2 ];
    player_gfx[ 0 ] = al_load_bitmap( "DATA/Gfxs/Santa_left.png" );
    player_gfx[ 1 ] = al_load_bitmap( "DATA/Gfxs/Santa_right.png" );

    do
    {
        ALLEGRO_EVENT ev ;

        al_wait_for_event(event_queue, &ev);

        if(ev.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            switch(ev.keyboard.keycode)
            {
            case ALLEGRO_KEY_UP:
                key[KEY_UP] = 1;
                break;
            case ALLEGRO_KEY_DOWN:
                key[KEY_DOWN] = 1;
                break;
            case ALLEGRO_KEY_LEFT:
                key[KEY_LEFT] = 1;
                break;
            case ALLEGRO_KEY_RIGHT:
                key[KEY_RIGHT] = 1;
                break;
            case ALLEGRO_KEY_ESCAPE:
                key[KEY_ESC] = 1 ;
                break;
            case ALLEGRO_KEY_SPACE:
                key[KEY_SPACE] = 1 ;
                break;
            case ALLEGRO_KEY_LCTRL:
            case ALLEGRO_KEY_RCTRL:
                key[KEY_CTRL] = 1 ;
            default:
                break;
            }
        }
        else if(ev.type == ALLEGRO_EVENT_KEY_UP)
        {
            switch(ev.keyboard.keycode)
            {
            case ALLEGRO_KEY_UP:
                key[KEY_UP] = 0;
                break;
            case ALLEGRO_KEY_DOWN:
                key[KEY_DOWN] = 0;
                break;
            case ALLEGRO_KEY_LEFT:
                key[KEY_LEFT] = 0;
                break;
            case ALLEGRO_KEY_RIGHT:
                key[KEY_RIGHT] =0;
                break;
            case ALLEGRO_KEY_ESCAPE:
                key[KEY_ESC] = 0 ;
                break;
            case ALLEGRO_KEY_SPACE:
                key[KEY_SPACE] = 0 ;
                break;
            case ALLEGRO_KEY_LCTRL:
            case ALLEGRO_KEY_RCTRL:
                key[KEY_CTRL] = 0 ;
            default:
                break;
            }
        }
        else if( ev.type == ALLEGRO_EVENT_TIMER )
        {
            if( al_get_timer_event_source( fps_timer ) == ev.any.source )
            {
                do_draw = 1 ;
            }
            else if( al_get_timer_event_source( logic_timer ) == ev.any.source )
            {
                do_logic = 1;
            }
        }
        else if( ev.type == ALLEGRO_EVENT_MOUSE_AXES )
        {
            mx = ev.mouse.x;
            my = ev.mouse.y;
        }
        else if( ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN )
        {
            if( ev.mouse.button == 1 )
                mouse_b1 = 1 ;
            if( ev.mouse.button == 2 )
                mouse_b2 = 1 ;
        }
        else if( ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP )
        {
            if( ev.mouse.button == 1 )
                mouse_b1 = 0 ;
            if( ev.mouse.button == 2 )
                mouse_b2 = 0 ;
        }


        if( do_logic == 1 )
        {
            /* Processing inputs */

            /* dev mod: right click to temporary delete a block
               left click to temporary add a block */
            int mouse_button = -1 ;
            if( mouse_b1==1 )
                mouse_button = 1 ;
            if( mouse_b2==1 )
                mouse_button = 2 ;
            if( mouse_button != -1 )
            {
                int x = 0, y = 0, px = 0, py = 0 ;
                PHYSICS position ;

                memcpy( &position, &player . physics, sizeof( PHYSICS ) );

                if( get_level_data( level, &position, mx - WIDTH/2, my - HEIGHT/2, &x, &y ) != -1 )
                {
                    if( get_level_data( level, &position, 0.0, 0.0, &px, &py ) != -1 )
                    {
                        if( x != px || y != py )
                        {
                            if( mouse_button == 1 && level -> cells[ x ][ y ] == 0 && used_magic_blocks < max_magic_blocks  )
                            {
                                level -> cells[ x ][ y ] = 3 ;
                                used_magic_blocks ++ ;
                            }
                            if( mouse_button == 2 && level -> cells[ x ][ y ] == 3 )
                            {
                                level -> cells[ x ][ y ] = 0 ;
                                used_magic_blocks -- ;
                            }
                        }
                    }
                }
            }

            player . attr . move = 0 ;


            if( player . physics . can_jump == 1 && ( key[ KEY_UP ] || key[ KEY_SPACE ] )  )
            {
                VECTOR3D_SET( player . physics . speed, player . physics . speed[ 0 ], player . physics . speed[ 1 ] - player . attr . yspeedinc, player . physics . speed[ 2 ] );
                if( player . physics . speed[ 1 ] < -player . attr . yspeedmax )
                    player . physics . speed[ 1 ] = -player . attr . yspeedmax ;

                player . physics . can_jump = 0 ;
                player . attr . move = 1 ;
            }

            /* go down like you want */
            if( key[ KEY_DOWN ] && player . physics . speed[ 1 ] < 0.0 )
            {
                VECTOR3D_SET( player . physics . speed, player . physics . speed[ 0 ], player . physics . speed[ 1 ] + player . attr . yspeedinc, player . physics . speed[ 2 ] );
                if( player . physics . speed[ 1 ] < -player . attr . yspeedmax )
                    player . physics . speed[ 1 ] = -player . attr . yspeedmax ;
                player . attr . move = 1 ;
            }
            else
            {
                if( player . physics . speed[ 1 ] < 0.0 )
                {
                    player . physics . speed[ 1 ] = player . physics . speed[ 1 ] + friction[ 1 ] * delta_time / 1000000.0 ;
                    if( player . physics . speed[ 1 ] > 0.0 )
                        player . physics . speed[ 1 ] = 0.0 ;
                }
            }



            if( key[ KEY_LEFT ] )
            {
                VECTOR3D_SET( player . physics . speed, player . physics . speed[ 0 ] - player . attr . xspeedinc, player . physics . speed[ 1 ], player . physics . speed[ 2 ] );
                if( player . physics . speed[ 0 ] < -player . attr . xspeedmax )
                    player . physics . speed[ 0 ] = -player . attr . xspeedmax ;
                player . attr . direction = 0 ;
                player . attr . move = 1 ;
            }
            else
            {
                if( player . physics . speed[ 0 ] < 0.0 )
                {
                    player . physics . speed[ 0 ] = player . physics . speed[ 0 ] + friction[ 0 ] * delta_time / 1000000.0 ;
                    if( player . physics . speed[ 0 ] > 0.0 )
                        player . physics . speed[ 0 ] = 0.0 ;
                }
            }

            if( key[ KEY_RIGHT ] )
            {
                VECTOR3D_SET( player . physics . speed, player . physics . speed[ 0 ] + player . attr . xspeedinc, player . physics . speed[ 1 ], player . physics . speed[ 2 ] );
                if( player . physics . speed[ 0 ] > player . attr . xspeedmax )
                    player . physics . speed[ 0 ] = player . attr . xspeedmax ;
                player . attr . direction = 1 ;
                player . attr . move = 1 ;
            }
            else
            {
                if( player . physics . speed[ 0 ] > 0.0 )
                {
                    player . physics . speed[ 0 ] = player . physics . speed[ 0 ] - friction[ 0 ] * delta_time / 1000000.0 ;
                    if( player . physics . speed[ 0 ] < 0.0 )
                        player . physics . speed[ 0 ] = 0.0 ;
                }
            }


            if( key[KEY_CTRL ] )
            {
                if( player . attr . action != 3 )
                    player . attr . action = 2 ;
            }
            else
            {
                player . attr . action = 0 ;
            }

            if( abs( player . physics . speed[ 0 ] ) < 0.5 )
                player . physics . speed[ 0 ] = 0.0 ;

            memcpy( &old_pos, &player . physics . position, sizeof( VECTOR3D ) );

            animate_physics( level, &player . physics, friction, delta_time );
            animate_monster( level, delta_time );


            VECTOR3D particle_move ;
            for( int it = 0 ; it < 3 ; it ++ )
                particle_move[ it ] = old_pos[ it ] - player . physics . position[ it ] ;

            move_particles( particle_system, particle_move[ 0 ], particle_move[ 1 ], particle_move[ 2 ] );

            /* add particles to player position , one blue , one red , one white/blue */
            if( ( player . attr . move != 0 || player . physics . can_jump == 0 ) || player . attr . action == 2 )
            {
                PHYSICS tmp_part ;
                int xpos = WIDTH / 2 ;
                int ypos = HEIGHT/ 2 ;
                int dir = -1 ;
                if( player . attr . direction == 1 )
                    dir = 1 ;
                if( player . attr . move == 0 )
                    dir = 1-rand()%3 ;
                VECTOR3D_SET( tmp_part . speed, -dir * ( 100 + rand()%200 ), (-20 + rand()%40 ), 0.0  );
                VECTOR3D_SET( tmp_part . position, xpos + 10 - rand()%20, ypos + 10 - rand()%20, 0.0  );
                VECTOR3D_SET( tmp_part . acceleration, 0.0, 0.0, 0.0 );
                VECTOR3D_SET( tmp_part . orientation, 0.0, 0.0, 0.0 );
                add_particle( particle_system, -1, NORMAL_PART, 100 + rand()%500, al_map_rgba(   55 + rand()%200,  55 + rand()%200, 55 + rand()%200, 100 + rand()%155 ), tmp_part );

                if( player . attr . action == 2 )
                {
                    player . attr . action = 3 ;
                    dir = -1 ;
                    if( player . attr . direction == 1 )
                        dir = 1 ;

                    for( int it = 0 ; it < 100 ; it ++ )
                    {

                        VECTOR3D_SET( tmp_part . speed, dir * ( 500 + rand()%500 ), (-50 + rand()%100 ), 0.0  );
                        VECTOR3D_SET( tmp_part . position, xpos + dir * rand()%20, ypos - rand()%64, 0.0  );
                        VECTOR3D_SET( tmp_part . acceleration, -dir * ( 500 + rand()%500 ), 0.0, 0.0 );
                        VECTOR3D_SET( tmp_part . orientation, 0.0, 0.0, 0.0 );

                        add_particle( particle_system, -1, NORMAL_PART, 100 + rand()%500, al_map_rgba(  55 + rand()%200, 55 + rand()%200, 10, 100 + rand()%155 ), tmp_part );

                    }
                    list_foreach( node, level -> monster_list )
                    {
                        MONSTER *npc = (MONSTER *)node -> ptr ;
                        int px = player . physics . position[ 0 ] ;
                        int py = player . physics . position[ 1 ] ;
                        int npx = npc -> physics . position[0];
                        int npy = npc -> physics . position[1];
                        if( dir > 0 )
                        {
                            if( npx >= px && npx <= ( ( px + 200 ) ) && npy >= ( py - 64 ) && npy <= ( py + 5 ) )
                            {
                                int pmult = 1 ;
                                npc -> attr . life -= 33 ;
                                if( npc -> attr . life <= 0 )
                                    pmult = 3 ;
                                npc -> physics . speed[ 0 ] = 50 + abs( npc -> physics . speed[ 0 ] ) ;
                                npc -> physics . speed[ 1 ] = -250 ;
                                npx =  WIDTH /2 - player . physics . position[ 0 ] + npc -> physics . position[0] ;
                                npy =  HEIGHT/2 - player . physics . position[ 1 ] + npc -> physics . position[1] ;
                                for( int it = 0 ; it < 10*pmult ; it ++ )
                                {
                                    VECTOR3D_SET( tmp_part . speed, 50 + rand()%100, -50 - rand()%100, 0.0  );
                                    VECTOR3D_SET( tmp_part . position, npx, npy, 0.0  );
                                    VECTOR3D_SET( tmp_part . acceleration, 0.0, 0.0, 0.0 );
                                    VECTOR3D_SET( tmp_part . orientation, 0.0, 0.0, 0.0 );
                                    add_particle( particle_system, -1, NORMAL_PART, 500 + rand()%500, al_map_rgba(  55 + rand()%200, 0, 0, 100 + rand()%155 ), tmp_part );
                                }
                            }
                        }
                        if( dir < 0 )
                        {
                            if( npx >= ( px - 200 ) && npx <= px && npy >= ( py - 64 ) && npy <= ( py + 5 ) )
                            {
                                int pmult = 1 ;
                                npc -> attr . life -= 33 ;
                                if( npc -> attr . life <= 0 )
                                    pmult = 3 ;
                                npc -> physics . speed[ 0 ] = -50 -abs( npc -> physics . speed[ 0 ] ) ;
                                npc -> physics . speed[ 1 ] = -250 ;
                                npx =  WIDTH /2 - player . physics . position[ 0 ] + npc -> physics . position[0] ;
                                npy =  HEIGHT/2 - player . physics . position[ 1 ] + npc -> physics . position[1] ;
                                for( int it = 0 ; it < 10*pmult; it ++ )
                                {
                                    VECTOR3D_SET( tmp_part . speed, -50 - rand()%100, -50 - rand()%100, 0.0  );
                                    VECTOR3D_SET( tmp_part . position, npx, npy, 0.0  );
                                    VECTOR3D_SET( tmp_part . acceleration, 0.0, 0.0, 0.0 );
                                    VECTOR3D_SET( tmp_part . orientation, 0.0, 0.0, 0.0 );
                                    add_particle( particle_system, -1, NORMAL_PART, 500 + rand()%500, al_map_rgba(  55 + rand()%200, 0, 0, 100 + rand()%155 ), tmp_part );
                                }
                            }
                        }
                    }
                }
            }
            list_foreach( node, level -> monster_list )
            {
                MONSTER *npc = (MONSTER *)node -> ptr ;
                int px = player . physics . position[ 0 ] ;
                int py = player . physics . position[ 1 ] ;
                int npx = npc -> physics . position[0];
                int npy = npc -> physics . position[1];
                if( npx >= px - 24 && npx <= px + 24 && npy >= py - 62 && npy <= py )
                {
                    player . attr . life -= 3 ;
                    if( player . attr . life < 0 )
                    {
                        player . attr . life  = 0 ;
                        DONE = 2 ;
                    }
                    if( npx < px )
                    {
                        npc -> physics . speed[0] = -abs(	npc -> physics . speed[0] ) - 10 ;
                        npc -> physics . speed[1] = -100 ;
                    }
                    else
                    {
                        npc -> physics . speed[0] = abs(	npc -> physics . speed[0] ) + 10 ;
                        npc -> physics . speed[1] = -100 ;
                    }
                    npx =  WIDTH /2 - player . physics . position[ 0 ] + npc -> physics . position[0] ;
                    npy =  HEIGHT/2 - player . physics . position[ 1 ] + npc -> physics . position[1] ;
                    for( int it = 0 ; it < 20; it ++ )
                    {
                        PHYSICS tmp_part ;
                        VECTOR3D_SET( tmp_part . speed, -50 - rand()%100, -50 - rand()%100, 0.0  );
                        VECTOR3D_SET( tmp_part . position, npx, npy, 0.0  );
                        VECTOR3D_SET( tmp_part . acceleration, 0.0, 0.0, 0.0 );
                        VECTOR3D_SET( tmp_part . orientation, 0.0, 0.0, 0.0 );
                        add_particle( particle_system, -1, NORMAL_PART, 500 + rand()%500, al_map_rgba(  55 + rand()%200, 0, 0, 100 + rand()%155 ), tmp_part );
                    }

                }
            }



            manage_particle( particle_system );

            int cellx = player . physics . position[ 0 ] / level -> tilew ;
            int celly = player . physics . position[ 1 ] / level -> tileh ;
            /* cell containing a rescue item */
            if( level -> cells[ cellx ][ celly ] >= 100 )
            {
                level -> cells[ cellx ][ celly ] = 0 ;
                nb_to_rescue -- ;
            }
            /* cell containing a wall item */
            if( level -> cells[ cellx ][ celly ] == 11 )
            {
                level -> cells[ cellx ][ celly ] = 0 ;
                max_magic_blocks ++ ;
            }
            /* cell containing an exit */
            if( level -> cells[ cellx ][ celly ] == -2 && nb_to_rescue == 0 )
                DONE = 1 ;
            do_logic = 0 ;
        }

        if( do_draw == 1 )
        {
            al_set_target_bitmap( scrbuf );
            al_clear_to_color( al_map_rgba( 0, 0, 0, 255 ) );

            draw_level( level, player . physics . position[ 0 ], player . physics . position[ 1 ], WIDTH, HEIGHT );

            int pw = al_get_bitmap_width(  player_gfx[ player .attr . direction ] );
            int ph = al_get_bitmap_height(  player_gfx[ player . attr . direction ] );

            al_draw_bitmap( player_gfx[ player . attr . direction ], WIDTH / 2 - pw/2, HEIGHT/2 - ph, 0);

            draw_particle( particle_system );

            al_acknowledge_resize( display );
            int w = al_get_display_width(  display );
            int h = al_get_display_height( display );

            al_set_target_bitmap( al_get_backbuffer( display ) );

            al_clear_to_color( al_map_rgba( 0, 0, 0, 255 ) );
            al_draw_bitmap( scrbuf, w/2 - al_get_bitmap_width( scrbuf ) /2, h/2 - al_get_bitmap_height( scrbuf ) / 2, 0 );

            /* mouse pointer */
            al_draw_line( mx - 5, my, mx + 5, my, al_map_rgb( 255, 0, 0 ), 1 );
            al_draw_line( mx, my + 5, mx, my - 5, al_map_rgb( 255, 0, 0 ), 1 );

            al_draw_filled_rectangle( 20, HEIGHT - 20, 20 + player . attr . life * 10, HEIGHT - 40, al_map_rgb( 255, 0, 0 ) );

            N_STR *nstr = NULL ;
            nstrprintf( nstr, "Life: %d Number of blocks available: %d Still to rescue: %d", player . attr . life, max_magic_blocks, nb_to_rescue );
            al_draw_text( font, al_map_rgb( 255, 0, 0 ), 10, 10, 0, _nstr( nstr ) );
            free_nstr( &nstr );

            al_flip_display();
            do_draw = 0 ;
        }

    }
    while( !key[KEY_ESC] && !DONE );

    N_STR *str = new_nstr( 100 );
    nstrcat_bytes( str, "--Santa To The Rescue--\n\nEND OF GAME\n\n" );
    if( DONE == 1 )
    {
        nstrcat_bytes( str, "YOU SAVED EVERYONE ! WELL PLAYED !\n" );
    }
    else
        nstrcat_bytes( str, "YOU LOST !\n" );



    al_show_native_message_box(  al_get_current_display(),
                                 "SantaToTheRescue",
                                 "Game Over", _nstr( str ), NULL, 0  );




    /******************************************************************************
     *                             EXITING                                        *
     ******************************************************************************/

    list_destroy( &active_object );

    return TRUE;

}



int choose_level()
{
    return TRUE ;
}

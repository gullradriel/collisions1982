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
#include "nilorea/n_particles.h"
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
        __attribute__ ((fallthrough));
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
    al_set_window_title( display, "Collision1982" );

    al_set_new_bitmap_flags( ALLEGRO_VIDEO_BITMAP );

    ALLEGRO_FONT *font = al_load_font( "DATA/2Dumb.ttf", 18, 0 );


    DONE = 0 ;

    double delta_time = 1000000.0/40.0 ;

    active_object = new_generic_list( -1 );

    PLAYER player ;

    LEVEL *level = NULL ;
    level = load_level( "DATA/Levels/level1.txt", "DATA/Levels/level1_ressources.txt", WIDTH, HEIGHT );

    VECTOR3D_SET( player . physics . position, level -> startx, level -> starty, 0.0 );
    VECTOR3D_SET( player . physics . speed,  0.0, 0.0, 0.0  );
    VECTOR3D_SET( player . physics . acceleration, 0.0, 0.0, 0.0 );
    VECTOR3D_SET( player . physics . orientation, 0.0, 0.0, 0.0 );

    player . physics . can_jump = 0, player . physics . sz = 3 ;
    player . attr . action = 0 ;
    player . attr . move = 0 ;
    player . attr . life = 100 ;
    player . attr . xp = 0 ;
    player . attr . level = 1 ;
    player . attr . xp_to_level = 1000 ;
    player . attr . direction = 0 ;
    player . attr . xspeedmax = 1000 ;
    player . attr . xspeedinc = 10 ;
    player . attr . yspeedmax = 500 ;
    player . attr . yspeedinc = 10 ;

    double shoot_rate_time = 0 ;

    enum APP_KEYS
    {
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ESC, KEY_SPACE, KEY_CTRL
    };
    int key[ 7 ] = {false,false,false,false,false,false,false};
    VECTOR3D old_pos ;
    VECTOR3D friction = { 0.0, 15.0, 0.0 };

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


        /* Processing inputs */

        /* dev mod: right click to temporary delete a block
           left click to temporary add a block */
        int mouse_button = -1 ;
        if( mouse_b1==1 )
            mouse_button = 1 ;
        if( mouse_b2==1 )
            mouse_button = 2 ;

        player . attr . move = 0 ;

        if( key[ KEY_UP ] )
        {
            VECTOR3D_SET( player . physics . speed, player . physics . speed[ 0 ], player . physics . speed[ 1 ] - player . attr . yspeedinc, player . physics . speed[ 2 ] );
            if( player . physics . speed[ 1 ] < -player . attr . yspeedmax )
                player . physics . speed[ 1 ] = -player . attr . yspeedmax ;

            player . physics . can_jump = 0 ;
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


        if( key[ KEY_DOWN ] )
        {
            VECTOR3D_SET( player . physics . speed, player . physics . speed[ 0 ], player . physics . speed[ 1 ] + player . attr . yspeedinc, player . physics . speed[ 2 ] );
            if( player . physics . speed[ 1 ] > player . attr . yspeedmax )
                player . physics . speed[ 1 ] = player . attr . yspeedmax ;
            player . physics . can_jump = 0 ;
            player . attr . move = 1 ;
        }
        else
        {
            if( player . physics . speed[ 1 ] > 0.0 )
            {
                player . physics . speed[ 1 ] = player . physics . speed[ 1 ] - friction[ 1 ] * delta_time / 1000000.0 ;
                if( player . physics . speed[ 1 ] < 0.0 )
                    player . physics . speed[ 1 ] = 0.0 ;
            }
        }

        if( key[ KEY_LEFT ] )
        {
            VECTOR3D_SET( player . physics . speed, player . physics . speed[ 0 ] - player . attr . xspeedinc, player . physics . speed[ 1 ], player . physics . speed[ 2 ] );
            if( player . physics . speed[ 0 ] < -player . attr . xspeedmax )
                player . physics . speed[ 0 ] = -player . attr . xspeedmax ;
            player . attr . direction = 1 ;
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


        if( key[KEY_CTRL ]  || mouse_button == 1 )
        {
            if( player . attr . action == 0 )
                player . attr . action = 2 ;

            if( player . attr . action == 3 )
            {
                if( shoot_rate_time > ( ( 11 - player . attr . level ) * 33333 ) )
                {
                    player . attr . action = 2 ;
                    shoot_rate_time -= ( ( 11 - player . attr . level ) * 33333 )  ;
                }
            }
        }
        else
        {
            shoot_rate_time = 0 ;
            player . attr . action = 0 ;
        }

        if( do_logic == 1 )
        {
            shoot_rate_time += delta_time ;
            if( abs( player . physics . speed[ 0 ] ) < 0.5 )
                player . physics . speed[ 0 ] = 0.0 ;

            memcpy( &old_pos, &player . physics . position, sizeof( VECTOR3D ) );
            animate_physics( level, &player . physics, friction, delta_time );

            animate_level( level, &player, delta_time );

            /* add particles to player position , one blue , one red , one white/blue */
            if( ( player . attr . move != 0 || player . physics . can_jump == 0 ) || player . attr . action == 2 )
            {

                PHYSICS tmp_part ;
                tmp_part . sz = 10 ;
                for( int it = 0 ; it < player . physics . speed[ 0 ] /100 ; it ++ )
                {
                    VECTOR3D_SET( tmp_part . speed,
                                  -player . physics . speed[ 0 ] + 100 - rand()%200,
                                  -player . physics . speed[ 1 ] + 100 - rand()%200,
                                  0.0  );
                    VECTOR3D_SET( tmp_part . position, player . physics . position[ 0 ], player . physics . position[ 1 ], 0.0  );
                    VECTOR3D_SET( tmp_part . acceleration, 0.0, 0.0, 0.0 );
                    VECTOR3D_SET( tmp_part . orientation, 0.0, 0.0, 0.0 );
                    add_particle( level -> particle_system_effects, -1, PIXEL_PART, 100 + rand()%500, 1+rand()%3,
                                  al_map_rgba(   55 + rand()%200,  55 + rand()%200, 55 + rand()%200, 10 + rand()%245 ), tmp_part );

                }

                if( player . attr . action == 2 )
                {
                    player . attr . action = 3 ;

                    double itx = 0, ity = 0, itz = 0, sz = 0;

                    itx = (double)(mx - level -> native_w / 2  ) - level -> tilew / 2 ;
                    ity = (double)(my - level -> native_h / 2  ) - level -> tileh / 2 ;
                    itz = 0 ;
                    sz = sqrt((itx * itx) + (ity * ity) + (itz * itz));
                    itx /= sz ;
                    ity /= sz ;
                    itz /= sz ;

                    for( int it = 0  ; it < player. attr . level ; it ++ )
                    {
                        double speed_mult = 500 +rand()%100 ;
                        double accuity = rand()%( 10* (11 - player . attr . level) );
                        VECTOR3D_SET( tmp_part . speed, accuity + player . physics . speed[ 0 ] + itx * speed_mult, accuity + player . physics . speed[ 1 ] + ity * speed_mult, player . physics . speed[ 2 ] + itz * speed_mult );
                        VECTOR3D_SET( tmp_part . position, player . physics . position[ 0 ], player . physics . position[ 1 ],  0.0  );
                        VECTOR3D_SET( tmp_part . acceleration, 0.0, 0.0, 0.0 );
                        VECTOR3D_SET( tmp_part . orientation, 0.0, 0.0, 0.0 );
                        int bw_color =  10 + rand()%200 ;
                        add_particle( level -> particle_system_bullets, -1, PIXEL_PART, 2000 , 5 + rand()%player.attr.level,  al_map_rgba(  bw_color, bw_color, bw_color, 255 - bw_color  ), tmp_part );

                    }

                }

                list_foreach( node, level -> monster_list )
                {
                    MONSTER *npc = (MONSTER *)node -> ptr ;
                    int px = player . physics . position[ 0 ] ;
                    int py = player . physics . position[ 1 ] ;
                    int npx = npc -> physics . position[0];
                    int npy = npc -> physics . position[1];

                    if( npx >= px - 32 && npx <= px + 32 && npy >= py - 32 && npy <= py + 32 )
                    {
                        player . attr . life -= 3 ;
                        npc -> attr . life = 0 ;

                        if( player . attr . life < 0 )
                        {
                            player . attr . life  = 0 ;
                            DONE = 2 ;
                        }
                        npx =  player . physics . position[ 0 ] ;
                        npy =  player . physics . position[ 1 ] ;
                        for( int it = 0 ; it < 20; it ++ )
                        {
                            PHYSICS tmp_part ;
                            tmp_part . sz = 5  ;
                            VECTOR3D_SET( tmp_part . speed, 100 - rand()%200, 100 - rand()%200, 0.0  );
                            VECTOR3D_SET( tmp_part . position, px, py, 0.0  );
                            VECTOR3D_SET( tmp_part . acceleration, 0.0, 0.0, 0.0 );
                            VECTOR3D_SET( tmp_part . orientation, 0.0, 0.0, 0.0 );
                            add_particle( level -> particle_system_effects, -1, PIXEL_PART, 500 + rand()%500, 1+rand()%5, al_map_rgba( 0 , 55 + rand()%200, 0, 100 + rand()%155 ), tmp_part );
                        }
                    }
                }
            }

            int cellx = player . physics . position[ 0 ] / level -> tilew ;
            int celly = player . physics . position[ 1 ] / level -> tileh ;
            if( cellx >= 0 && cellx < level -> w && celly >= 0 && celly < level -> h )
            {
                /* cell containing a bonus item */
                if( level -> cells[ cellx ][ celly ] == 11 )
                {
                    level -> cells[ cellx ][ celly ] = 0 ;
                }
                /* cell containing an exit */
                if( level -> cells[ cellx ][ celly ] == -2 )
                    DONE = 1 ;
            }
            do_logic = 0 ;
        }

        if( do_draw == 1 )
        {
            static int green_it = 100 ;
            al_set_target_bitmap( scrbuf );
            al_clear_to_color( al_map_rgba( 0, 0, 0, 255 ) );

            draw_level( level, player . physics . position[ 0 ], player . physics . position[ 1 ], WIDTH, HEIGHT );

            green_it +=15;
            if( green_it > 255 )
                green_it = 100 ;
            al_draw_filled_circle( WIDTH / 2, HEIGHT/2, 20 + green_it / 20,  al_map_rgb( 255 - (255 *player . attr . life), green_it - (green_it *(100- player . attr . life) ), 0 ) );

            al_acknowledge_resize( display );
            int w = al_get_display_width(  display );
            int h = al_get_display_height( display );

            al_set_target_bitmap( al_get_backbuffer( display ) );

            al_clear_to_color( al_map_rgba( 0, 0, 0, 255 ) );
            al_draw_bitmap( scrbuf, w/2 - al_get_bitmap_width( scrbuf ) /2, h/2 - al_get_bitmap_height( scrbuf ) / 2, 0 );

            /* mouse pointer */
            al_draw_line( mx - 5, my, mx + 5, my, al_map_rgb( 255, 0, 0 ), 1 );
            al_draw_line( mx, my + 5, mx, my - 5, al_map_rgb( 255, 0, 0 ), 1 );

            /* speed meter */
            al_draw_filled_rectangle( 20 , h/2 , 25 , h/2 + player . physics . speed[ 1 ] , al_map_rgba( 255 , 255 , 255 , 255 ) );
            al_draw_filled_rectangle( w/2 , h -20 , w/2 + player . physics . speed[ 0 ] , h-25, al_map_rgba( 255 , 255 , 255 , 255 ) );


            N_STR *nstr = NULL ;
            nstrprintf( nstr, "Life: %d %%", player . attr . life );
            al_draw_text( font, al_map_rgb( 255, 0, 0 ), 20, HEIGHT - 20, 0, _nstr( nstr ) );
            free_nstr( &nstr );

            al_flip_display();
            do_draw = 0 ;
        }

    }
    while( !key[KEY_ESC] && !DONE );

    N_STR *str = new_nstr( 100 );
    nstrcat_bytes( str, "Collision1982 " );
    if( DONE == 1 )
    {
        nstrcat_bytes( str, "YOU WIN !" );
    }
    else
        nstrcat_bytes( str, "YOU LOST !" );



    al_show_native_message_box(  al_get_current_display(),
                                 "Collision1982",
                                 "Game Over", _nstr( str ), NULL, 0  );




    /******************************************************************************
     *                             EXITING                                        *
     ******************************************************************************/

    list_destroy( &active_object );

    return 0;

}



int choose_level()
{
    return TRUE ;
}

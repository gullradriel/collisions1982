/**\file level
*
*  level file for hacks
*
*\author Castagnier Mickaël aka Gull Ra Driel
*
*\version 1.0
*
*\date 30/12/2016 based on 05/24/2014 version
*
*/



#ifndef LEVEL_HEADER_FOR_HACKS
#define LEVEL_HEADER_FOR_HACKS

#ifdef __cplusplus
extern "C" {
#endif

#include "nilorea/n_common.h"
#include "nilorea/n_log.h"
#include "nilorea/n_str.h"
#include "nilorea/n_hash.h"
#include "nilorea/n_anim.h"
#include "nilorea/n_particles.h"


#define ACTION_MOVE 2
#define ACTION_ATTACK 4

#define PASSIVE_MONSTER 0
#define AGRESSIVE_MONSTER 1
#define BIGBOSS_MONSTER 2


typedef struct ATTRIBUTES
{
    int	life,
        type,
        action,
        move,
        xp,
        xp_to_level,
        level,
        direction,
        xspeedmax,
        yspeedmax,
        xspeedinc,
        yspeedinc;
} ATTRIBUTES ;

typedef struct MONSTER
{
    /*! gfx */
    ANIM_DATA anim ;

    /* physic properties */
    PHYSICS	physics ;
    int action ;

    /* characteristics */
    ATTRIBUTES attr ;
} MONSTER ;

typedef struct PLAYER
{
    PHYSICS physics ;
    ANIM_DATA anim ;
        ATTRIBUTES attr ;
} PLAYER;

MONSTER *new_monster( int life, int type, PHYSICS physics );

typedef struct LEVEL
{

    int **cells,
        w,
        h,
        tilew,
        tileh,
        native_w,
        native_h,
        exitx,
        exity;

    double startx, starty ;

    LIST *monster_list ;

    ALLEGRO_BITMAP *tiles[ 4 ];

    PARTICLE_SYSTEM *particle_system_effects ;
    PARTICLE_SYSTEM *particle_system_bullets ;

} LEVEL ;

int get_level_data( LEVEL *level, PHYSICS *physics, int mx, int my, int *x, int *y );
LEVEL *load_level( char *file, char *resfile, int w, int h );
int draw_level( LEVEL *lvl, int x, int y, int w, int h);
int test_coord( LEVEL *level, PHYSICS *physics, VECTOR3D fricton, int off_x, int off_y );
int animate_physics( LEVEL *level, PHYSICS *physics, VECTOR3D friction, double delta_t );
int animate_level( LEVEL *level, PLAYER *player, double delta_t );



#ifdef __cplusplus
}
#endif
#endif

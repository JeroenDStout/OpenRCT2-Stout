#include "peepex.h"
#include "staff.h"
#include "../ride/ride.h"
#include "../world/footpath.h"
#include "../world/entrance.h"
#include "../scenario/scenario.h"
#include "../config/Config.h"
#include "../audio/audio.h"
   
    /* witness
     */

void peepex_make_witness(rct_peep *peep, uint16 sprite)
{
    log_warning("witness");

    peep->state = PEEP_STATE_WITNESSING_EVENT;
    peep->peepex_follow_target = sprite;
    peep->peepex_event_countdown = 3 + scenario_rand_max(16);

    if (peep->type == PEEP_TYPE_STAFF) {
        peep->peepex_event_countdown   = 3;
    }
}

void peepex_update_witness(rct_peep *peep)
{
    bool      stopFollowing         = false;
    bool      closeEnoughForReact   = false;

    peepex_follow_instr instr;
    instr.attempt_min_distance              = 10*10;
    instr.attempt_max_distance              = 64*64;
    instr.target_forward_offset             = 0;
    instr.out_facing_direction              = 0;
    instr.crowd_weight_in_percent           = 100;
    instr.base_gradient_weight_in_percent   = 0;

	rct_sprite* sprite = get_sprite(peep->peepex_follow_target);
    if (sprite->unknown.sprite_identifier != SPRITE_IDENTIFIER_PEEP) {
        stopFollowing = true;
    }

    if (!stopFollowing) {
        peepex_update_following(peep, &instr);
    
	    if (instr.out_target_lost) {
            stopFollowing = true;
        }
        
        closeEnoughForReact     = instr.out_comfortable_position;
    }

    if (!stopFollowing) {
        if (closeEnoughForReact) {
                // Put us at rest
            if (peep->action > PEEP_ACTION_NONE_1) {
                peep->action = PEEP_ACTION_NONE_1;
                peep->next_action_sprite_type = 2;
                invalidate_sprite_2((rct_sprite*)peep);
            }

                // Pick from a variety of reactions
            if (peep->type == PEEP_TYPE_GUEST && peep->action >= PEEP_ACTION_NONE_1 && (gScenarioTicks + peep->id) % 21 == 0) {
                peep->sprite_direction = instr.out_facing_direction * 8;
                peepex_event_broadcast_instr e = create_peepex_event_broadcast_instr();

                switch (scenario_rand_max(8)) {
                case 0:
                    invalidate_sprite_2((rct_sprite*)peep);
                    peep->action = PEEP_ACTION_TAKE_PHOTO;
                    peep->action_frame = 0; 
                    peep->action_sprite_image_offset = 0;
                    peep_update_current_action_sprite_type(peep);
                    invalidate_sprite_2((rct_sprite*)peep);
                    e.broadcast_type = PEEPEX_BROADCAST_EVENT_GENERIC_VISUAL_ODDITY;
                    e.primary_peep   = peep;
                    peepex_broadcast_event(&e);
                    break;
                };
            }
        }

            // Tick us down until we lose interest in this event
        if (gScenarioTicks % 11 == 0) {
            if (peep->peepex_event_countdown == 0)
                stopFollowing = true;
            peep->peepex_event_countdown -= 1;
        }
    }

        // It is over, move along
    if (stopFollowing) {
        log_warning("Event release");

        peepex_return_to_walking(peep);
    }
}

    /*  Hamelin (entertainer following) behaviour. Mostly just tell peepex_update_following what to do,
     *   and if the peep is close enough do a random assortment of cheer-like behaviour.
     *  Check against the entertainer to see if we can become their favourite peep - this allows us to be close
     *   to them until another peep bumps us away! We arrange that from here because entertainers are not
     *   actually aware of having fans.
     */

void peepex_make_hamelin(rct_peep *peep, rct_peep *hamelin)
{
    log_warning("hamelin");

    peep->state = PEEP_STATE_FOLLOWING_HAMELIN;
    peep->peepex_follow_target = hamelin->sprite_index;
    peep->peepex_hamelin_countdown = 10 + scenario_rand_max(128);
    peep->peepex_interest_in_entertainers = ((uint16)(peep->peepex_interest_in_entertainers) * 0xFF) * scenario_rand_max(0xFF);

    if (peep->type == PEEP_TYPE_STAFF) {
        peep->peepex_hamelin_countdown          = 10;
        peep->peepex_interest_in_entertainers   = 0;
    }
}

void peepex_update_hamelin(rct_peep *peep)
{
    bool      stopFollowing         = false;
	rct_peep* target_peep           = 0;
    bool      closeEnoughForCheer   = false;
    bool      peepHasBeenChosen     = false;

    peepex_follow_instr instr;
    instr.attempt_min_distance              = 20*20;
    instr.attempt_max_distance              = 64*64;
    instr.target_forward_offset             = 32;
    instr.out_facing_direction              = 0;
    instr.crowd_weight_in_percent           = 100;
    instr.base_gradient_weight_in_percent   = 100;

	rct_sprite* sprite = get_sprite(peep->peepex_follow_target);
    if (sprite->unknown.sprite_identifier != SPRITE_IDENTIFIER_PEEP) {
        stopFollowing = true;
    }

    if (!stopFollowing) {
	    target_peep = (rct_peep*)sprite;

            // We are the chosen one!
        if (target_peep->peepex_following_flags == peep->id) {
            instr.attempt_min_distance      = 4*4;
            instr.attempt_max_distance      = 12*12;
            instr.target_forward_offset     = 24;
            instr.crowd_weight_in_percent   = 10;
            peepHasBeenChosen               = true;
        }

        peepex_update_following(peep, &instr);
    
	    if (instr.out_target_lost) {
            stopFollowing = true;
        }
        
        closeEnoughForCheer     = instr.out_comfortable_position;
    }

    if (!stopFollowing) {
        if (closeEnoughForCheer) {
            if (((gScenarioTicks + target_peep->id) % 100) == 0) {
                if (scenario_rand_max(16) == 0)
                    target_peep->peepex_following_flags = peep->id;
            }

                // Put us at rest
            if (peep->action > PEEP_ACTION_NONE_1) {
                peep->action = PEEP_ACTION_NONE_1;
                peep->next_action_sprite_type = 2;
                invalidate_sprite_2((rct_sprite*)peep);
            }

                // Pick from a variety of very cheerful reactions
            if (peep->type == PEEP_TYPE_GUEST && peep->action >= PEEP_ACTION_NONE_1 && (gScenarioTicks + peep->id) % 21 == 0) {
                peep->sprite_direction = instr.out_facing_direction * 8;
                
                    // Boost happiness
                peep->happiness_target = min(PEEP_MAX_HAPPINESS, peep->happiness_target + (peepHasBeenChosen? 30 : 10));
                peepex_event_broadcast_instr e = create_peepex_event_broadcast_instr();

                sint32 laugh;
                switch (scenario_rand_max(peepHasBeenChosen? 8 : 16)) {
                case 0:
                    invalidate_sprite_2((rct_sprite*)peep);
                    peep->action = PEEP_ACTION_JUMP;
                    peep->action_frame = 0;
                    peep->action_sprite_image_offset = 0;
                    peep_update_current_action_sprite_type(peep);
                    invalidate_sprite_2((rct_sprite*)peep);
                    e.broadcast_type = PEEPEX_BROADCAST_EVENT_HAMELIN_SNARE;
                    e.primary_peep   = peep;
                    peepex_broadcast_event(&e);
                    break;
                case 1:
                case 2:
                case 3:
                    invalidate_sprite_2((rct_sprite*)peep);
                    peep->action = PEEP_ACTION_TAKE_PHOTO;
                    peep->action_frame = 0; 
                    peep->action_sprite_image_offset = 0;
                    peep_update_current_action_sprite_type(peep);
                    invalidate_sprite_2((rct_sprite*)peep);
                    break;
                case 4:
                    invalidate_sprite_2((rct_sprite*)peep);
                    peep->action = PEEP_ACTION_WAVE;
                    peep->action_frame = 0; 
                    peep->action_sprite_image_offset = 0;
                    peep_update_current_action_sprite_type(peep);
                    invalidate_sprite_2((rct_sprite*)peep);
                    break;
                case 5:
                    invalidate_sprite_2((rct_sprite*)peep);
                    peep->action = PEEP_ACTION_JOY;
                    peep->action_frame = 0; 
                    peep->action_sprite_image_offset = 0;
                    peep_update_current_action_sprite_type(peep);
                    invalidate_sprite_2((rct_sprite*)peep);
                    break;
                case 6:
                    laugh = scenario_rand() & 31;
                    if (laugh < 3)
                        audio_play_sound_at_location(SOUND_LAUGH_1 + laugh, peep->x, peep->y, peep->z);
                    e.broadcast_type = PEEPEX_BROADCAST_EVENT_HAMELIN_SNARE;
                    e.primary_peep   = peep;
                    peepex_broadcast_event(&e);
                    break;
                };
            }
        }

            // Tick us down until we lose interest in this peep
        if (gScenarioTicks % 11 == 0) {
            if (peep->peepex_hamelin_countdown == 0)
                stopFollowing = true;
            peep->peepex_hamelin_countdown -= 1;
        }
    }

        // It is over, move along
    if (stopFollowing) {
        log_warning("Hamelin release");

        peepex_return_to_walking(peep);
        if (peep->type == PEEP_TYPE_GUEST)
            peep->peepex_hamelin_countdown = 5 + scenario_rand_max(64);
        else
            peep->peepex_hamelin_countdown = 128;
        peep->state = PEEP_STATE_WALKING;
        peep->peepex_follow_target = 0;
        peep->peepex_hamelin_countdown = 5 + scenario_rand_max(64);
    }
}

    /*  
     */

void peepex_update_security_chasing(rct_peep *peep)
{
    bool      stopFollowing         = false;
	rct_peep* target_peep           = 0;
    bool      closeEnoughForArrest  = false;

    peepex_follow_instr instr;
    instr.attempt_min_distance              = 8*8;
    instr.attempt_max_distance              = 8*8;
    instr.target_forward_offset             = 16;
    instr.out_facing_direction              = 0;
    instr.crowd_weight_in_percent           = 100;
    instr.base_gradient_weight_in_percent   = 100;

	rct_sprite* sprite = get_sprite(peep->peepex_follow_target);
    if (sprite->unknown.sprite_identifier != SPRITE_IDENTIFIER_PEEP) {
        stopFollowing = true;
    }

    if (!stopFollowing) {
	    target_peep = (rct_peep*)sprite;

        peepex_update_following(peep, &instr);
    
	    if (instr.out_target_lost) {
            stopFollowing = true;
        }
        
        closeEnoughForArrest     = instr.out_comfortable_position;
    }

    if (!stopFollowing) {
        if (target_peep->state != PEEP_STATE_ESCORTED_BY_STAFF &&
            !(target_peep->flags & PEEP_FLAGS_LEAVING_PARK)) {
            if (closeEnoughForArrest) {
                log_warning("Book 'em...");
                peep->peepex_arrest_countdown   = 10 + scenario_rand_max(0x32);
                target_peep->peepex_follow_target = peep->sprite_index;
                target_peep->state = PEEP_STATE_ESCORTED_BY_STAFF;
                peep_leave_park(target_peep);
            }
        }
        else {
            if (closeEnoughForArrest) {
                if (peep->action > PEEP_ACTION_NONE_1) {
                    peep->action = PEEP_ACTION_NONE_1;
                    peep->next_action_sprite_type = 2;
                    invalidate_sprite_2((rct_sprite*)peep);
                }
            }
        }
            // Tick us down until we make a decision
        if (peep->peepex_arrest_countdown > 0 && gScenarioTicks % 11 == 0) {
            peep->peepex_arrest_countdown -= 1;
            if (peep->peepex_arrest_countdown <= 1) {
                peep->state = PEEP_STATE_SECURITY_ESCORTING_OUT;
            }
        }
    }

        // It is over, move along
    if (stopFollowing) {
        log_warning("We lost them!");
        peep->state = PEEP_STATE_PATROLLING;
        peep->peepex_follow_target = 0;
    }
}

    /*  
     */

void peepex_update_escorted_by_staff(rct_peep *peep)
{
    bool      stopFollowing         = false;
	rct_peep* target_peep           = 0;
    bool      arrested              = false;
    bool      closeEnoughForActions = false;

    peepex_follow_instr instr;
    instr.attempt_min_distance      = 8*8;
    instr.attempt_max_distance      = 8*8;
    instr.target_forward_offset     = 16;
    instr.out_facing_direction      = 0;
    instr.crowd_weight_in_percent   = 100;

	rct_sprite* sprite = get_sprite(peep->peepex_follow_target);
    if (sprite->unknown.sprite_identifier != SPRITE_IDENTIFIER_PEEP) {
        stopFollowing = true;
    }

    if (!stopFollowing) {
	    target_peep = (rct_peep*)sprite;
        
        if (target_peep->type == PEEP_TYPE_STAFF) {
            if (target_peep->staff_type == STAFF_TYPE_SECURITY) {
                    // Apprently we've been arrested!
                arrested    = true;

                if (target_peep->state != PEEP_STATE_SECURITY_CHASING &&
                    target_peep->state != PEEP_STATE_SECURITY_ESCORTING_OUT) {
                    stopFollowing = true;
                }
            }
        }

        peepex_update_following(peep, &instr);
    
	    if (instr.out_target_lost) {
            stopFollowing = true;
        }
        
        closeEnoughForActions     = instr.out_comfortable_position;
    }

    if (!stopFollowing) {
        if (closeEnoughForActions) {
                // Put us at rest
            if (peep->action > PEEP_ACTION_NONE_1) {
                peep->action = PEEP_ACTION_NONE_1;
                peep->next_action_sprite_type = 2;
                invalidate_sprite_2((rct_sprite*)peep);
            }

            if (arrested && target_peep->state == PEEP_STATE_SECURITY_CHASING) {
                    // Pick from a variety of unhappy reactions
                if (peep->action >= PEEP_ACTION_NONE_1 && (gScenarioTicks + peep->id) % 11 == 0) {
                    peep->sprite_direction = instr.out_facing_direction * 8;
                
                    switch (scenario_rand_max(16)) {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        invalidate_sprite_2((rct_sprite*)peep);
                        peep->action = PEEP_ACTION_SHAKE_HEAD;
                        peep->action_frame = 0; 
                        peep->action_sprite_image_offset = 0;
                        peep_update_current_action_sprite_type(peep);
                        invalidate_sprite_2((rct_sprite*)peep);
                        break;
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        invalidate_sprite_2((rct_sprite*)peep);
                        peep->action = PEEP_ACTION_EMPTY_POCKETS;
                        peep->action_frame = 0; 
                        peep->action_sprite_image_offset = 0;
                        peep_update_current_action_sprite_type(peep);
                        invalidate_sprite_2((rct_sprite*)peep);
                        break;
                    case 8:
                        invalidate_sprite_2((rct_sprite*)peep);
                        peep->action = PEEP_ACTION_CHECK_TIME;
                        peep->action_frame = 0; 
                        peep->action_sprite_image_offset = 0;
                        peep_update_current_action_sprite_type(peep);
                        invalidate_sprite_2((rct_sprite*)peep);
                        break;
                    };
                }
            }
        }
    }

        // It is over, we got away!
    if (stopFollowing) {
        log_warning("We got away!");
        peep->state = PEEP_STATE_WALKING;
        peep->peepex_follow_target = 0;
    }
}

void peepex_make_security_escort_out(rct_peep *peep)
{
    log_warning("escort out");
    peep_reset_pathfind_goal(peep);
}

void peepex_update_security_escorting_out(rct_peep *peep)
{
    bool stopFollowing = false;
    bool needWait = false;

    rct_peep *target_peep = 0;

    sint16 x, y, z, xy_distance;
    rct_map_element *mapElement;
    
	rct_sprite* sprite = get_sprite(peep->peepex_follow_target);
    if (sprite->unknown.sprite_identifier != SPRITE_IDENTIFIER_PEEP) {
        stopFollowing = true;
    }
    if (!stopFollowing) {
	    target_peep = (rct_peep*)sprite;
        sint16 dist_x = target_peep->x - peep->x;
        sint16 dist_y = target_peep->y - peep->y;
        if (dist_x * dist_x + dist_y * dist_y > 16*16) {
	        mapElement = map_get_path_element_below_or_at(peep->x / 32, peep->y / 32, (peep->z >> 3) + 2);
            if (!mapElement) {
                log_warning("We lost the path");
                stopFollowing = true;
            }
            needWait = true;
        }
    }
    if (!stopFollowing && !needWait) {
        if (peep_update_action(&x, &y, &xy_distance, peep)){
                // We have some distance to move, so handle paths and slopes
	        mapElement = map_get_path_element_below_or_at(x / 32, y / 32, (peep->z >> 3) + 2);
            if (mapElement) {
                peep->next_z        = mapElement->base_height;
                peep->next_var_29   = mapElement->properties.path.type & 7;
            }
            z = peep_get_height_on_slope(peep, x, y);
            sprite_move(x, y, z, (rct_sprite*)peep);
        }
        else {
            peep->next_x = peep->x & 0xFFE0;
            peep->next_y = peep->y & 0xFFE0;
            peep->next_z = peep->z >> 3;

                // lazily just get the nearest constantly
            uint8 chosenEntrance = get_nearest_park_entrance_index(peep->next_x, peep->next_y);
        
            x = gParkEntrances[chosenEntrance].x;
            y = gParkEntrances[chosenEntrance].y;
            z = gParkEntrances[chosenEntrance].z;

            sint16 distX = x - peep->x;
            sint16 distY = y - peep->y;

            if (distX*distX + distY*distY < 64 * 64) {
                peep_reset_pathfind_goal(target_peep);
                target_peep->state = PEEP_STATE_WALKING;
                peep->state = PEEP_STATE_PATROLLING;
                    log_warning("the law wins!!!");
            }

            gPeepPathFindGoalPosition = (rct_xyz16){ x, y, z >> 3 };
        
                    log_warning("try");

            sint32 direction = peep_pathfind_choose_direction(peep->next_x, peep->next_y, peep->next_z, peep);
            if (direction == -1) {
	            mapElement = map_get_path_element_below_or_at(peep->x / 32, peep->y / 32, peep->z >> 3);

                if (mapElement) {
                    direction = peepex_direction_from_xy(x - peep->x, y - peep->y);
            
                    log_warning("no dir %i", direction);

                    if (mapElement->properties.path.edges & (1 << direction)) {
                        log_warning("one tile %i %i %i %i", peep->x, peep->y, peep->destination_x, peep->destination_y);
                        peep->direction = direction;
                        peep_move_one_tile(direction, peep);
                    }
                    else {
                        log_warning("aimless");
                        guest_path_find_aimless(peep, mapElement->properties.path.edges);
                    }
                }
                else {
                    guest_surface_path_finding(peep);
                }
            }
            else {
                log_warning("dir %i", direction);

                peep_move_one_tile(direction, peep);
            }
        }
    }
    else {
        if (peep->action > PEEP_ACTION_NONE_1) {
            peep->action = PEEP_ACTION_NONE_1;
            peep->next_action_sprite_type = 2;
            invalidate_sprite_2((rct_sprite*)peep);
        }
    }

        // It is over, move along
    if (stopFollowing) {
        log_warning("We lost them!");
        peep->state = PEEP_STATE_PATROLLING;
        peep->peepex_follow_target = 0;
    }
}

void peepex_return_to_walking(rct_peep *peep)
{
    peep->peepex_follow_target = 0;

    peep->state = PEEP_STATE_WALKING;
    if (peep->type == PEEP_TYPE_STAFF) {
        peep->state = PEEP_STATE_PATROLLING;
    }
}
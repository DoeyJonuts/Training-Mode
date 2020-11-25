#include "ledgedash.h"
static char nullString[] = " ";

static GXColor tmgbar_black = {40, 40, 40, 255};
static GXColor tmgbar_grey = {80, 80, 80, 255};
static GXColor tmgbar_blue = {128, 128, 255, 255};
static GXColor tmgbar_green = {128, 255, 128, 255};
static GXColor tmgbar_yellow = {255, 255, 128, 255};
static GXColor tmgbar_red = {255, 128, 128, 255};
static GXColor tmgbar_indigo = {255, 128, 255, 255};
static GXColor tmgbar_white = {255, 255, 255, 255};
static GXColor *tmgbar_colors[] = {
    &tmgbar_black,
    &tmgbar_grey,
    &tmgbar_green,
    &tmgbar_yellow,
    &tmgbar_indigo,
    &tmgbar_white,
    &tmgbar_red,
};

// Main Menu
static char **LdshOptions_Barrel[] = {"Off", "Stationary", "Move"};
static char **LdshOptions_HUD[] = {"On", "Off"};
static EventOption LdshOptions_Main[] = {
    // Target
    {
        .option_kind = OPTKIND_STRING,               // the type of option this is; menu, string list, integers list, etc
        .value_num = sizeof(LdshOptions_Barrel) / 4, // number of values for this option
        .option_val = 0,                             // value of this option
        .menu = 0,                                   // pointer to the menu that pressing A opens
        .option_name = "Target",                     // pointer to a string
        .desc = "Enable a target to attack.",        // string describing what this option does
        .option_values = LdshOptions_Barrel,         // pointer to an array of strings
        .onOptionChange = 0,
    },
    // HUD
    {
        .option_kind = OPTKIND_STRING,            // the type of option this is; menu, string list, integers list, etc
        .value_num = sizeof(LdshOptions_HUD) / 4, // number of values for this option
        .option_val = 0,                          // value of this option
        .menu = 0,                                // pointer to the menu that pressing A opens
        .option_name = "HUD",                     // pointer to a string
        .desc = "Toggle visibility of the HUD.",  // string describing what this option does
        .option_values = LdshOptions_HUD,         // pointer to an array of strings
        .onOptionChange = 0,
    },
    // Help
    {
        .option_kind = OPTKIND_FUNC,                                                                                                                                                                                       // the type of option this is; menu, string list, integers list, etc
        .value_num = 0,                                                                                                                                                                                                    // number of values for this option
        .option_val = 0,                                                                                                                                                                                                   // value of this option
        .menu = 0,                                                                                                                                                                                                         // pointer to the menu that pressing A opens
        .option_name = "Help",                                                                                                                                                                                             // pointer to a string
        .desc = "L-canceling is performed by pressing L, R, or Z up to \n7 frames before landing from a non-special aerial\nattack. This will cut the landing lag in half, allowing \nyou to act sooner after attacking.", // string describing what this option does
        .option_values = 0,                                                                                                                                                                                                // pointer to an array of strings
        .onOptionChange = 0,
    },
    // Exit
    {
        .option_kind = OPTKIND_FUNC,                     // the type of option this is; menu, string list, integers list, etc
        .value_num = 0,                                  // number of values for this option
        .option_val = 0,                                 // value of this option
        .menu = 0,                                       // pointer to the menu that pressing A opens
        .option_name = "Exit",                           // pointer to a string
        .desc = "Return to the Event Selection Screen.", // string describing what this option does
        .option_values = 0,                              // pointer to an array of strings
        .onOptionChange = 0,
        .onOptionSelect = Event_Exit,
    },
};
static EventMenu LdshMenu_Main = {
    .name = "Ledgedash Training",                                 // the name of this menu
    .option_num = sizeof(LdshOptions_Main) / sizeof(EventOption), // number of options this menu contains
    .scroll = 0,                                                  // runtime variable used for how far down in the menu to start
    .state = 0,                                                   // bool used to know if this menu is focused, used at runtime
    .cursor = 0,                                                  // index of the option currently selected, used at runtime
    .options = &LdshOptions_Main,                                 // pointer to all of this menu's options
    .prev = 0,                                                    // pointer to previous menu, used at runtime
};

// Init Function
void Event_Init(GOBJ *gobj)
{
    LedgedashData *event_data = gobj->userdata;

    // theres got to be a better way to do this...
    event_vars = *event_vars_ptr;

    // get assets
    event_data->assets = File_GetSymbol(event_vars->event_archive, "ldshData");

    // Init HUD
    Ledgedash_HUDInit(event_data);

    // Init Fighter
    Ledgedash_FtInit(event_data);

    Stage *stage = stc_stage;
    float *unk_cam = 0x803bcca0;
    stc_stage->fov_r = 0; // no camera rotation
    stc_stage->x28 = 1;   // pan value?
    stc_stage->x2c = 1;   // pan value?
    stc_stage->x30 = 1;   // pan value?
    stc_stage->x34 = 130; // zoom out
    unk_cam[0x40 / 4] = 30;

    return;
}
// Think Function
void Event_Think(GOBJ *event)
{
    LedgedashData *event_data = event->userdata;

    // get fighter data
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;
    HSD_Pad *pad = PadGet(hmn_data->player_controller_number, PADGET_ENGINE);

    Ledgedash_HUDThink(event_data, hmn_data);
    Ledgedash_FighterThink(event_data, hmn);

    return;
}
void Event_Exit()
{
    Match *match = MATCH;

    // end game
    match->state = 3;

    // cleanup
    Match_EndVS();

    // unfreeze
    HSD_Update *update = HSD_UPDATE;
    update->pause_develop = 0;
    return;
}

// L-Cancel functions
void Ledgedash_HUDInit(LedgedashData *event_data)
{

    // create hud cobj
    GOBJ *hudcam_gobj = GObj_Create(19, 20, 0);
    ArchiveInfo **ifall_archive = 0x804d6d5c;
    COBJDesc ***dmgScnMdls = File_GetSymbol(*ifall_archive, 0x803f94d0);
    COBJDesc *cam_desc = dmgScnMdls[1][0];
    COBJ *hud_cobj = COBJ_LoadDesc(cam_desc);
    // init camera
    GObj_AddObject(hudcam_gobj, R13_U8(-0x3E55), hud_cobj);
    GOBJ_InitCamera(hudcam_gobj, LedgedashHUDCamThink, 7);
    hudcam_gobj->cobj_links = 1 << 18;

    GOBJ *hud_gobj = GObj_Create(0, 0, 0);
    event_data->hud.gobj = hud_gobj;
    // Load jobj
    JOBJ *hud_jobj = JOBJ_LoadJoint(event_data->assets->hud);
    GObj_AddObject(hud_gobj, 3, hud_jobj);
    GObj_AddGXLink(hud_gobj, GXLink_Common, 18, 80);

    // account for widescreen
    float aspect = (hud_cobj->projection_param.perspective.aspect / 1.216667) - 1;
    JOBJ *this_jobj;
    JOBJ_GetChild(hud_jobj, &this_jobj, 1, -1);
    this_jobj->trans.X += (this_jobj->trans.X * aspect);
    JOBJ_SetMtxDirtySub(hud_jobj);

    // create text canvas
    int canvas = Text_CreateCanvas(2, hud_gobj, 14, 15, 0, 18, 81, 19);
    event_data->hud.canvas = canvas;

    // init text
    Text **text_arr = &event_data->hud.text_angle;
    for (int i = 0; i < 2; i++)
    {

        // Create text object
        Text *hud_text = Text_CreateText(2, canvas);
        text_arr[i] = hud_text;
        hud_text->kerning = 1;
        hud_text->align = 1;
        hud_text->use_aspect = 1;

        // Get position
        Vec3 text_pos;
        JOBJ *text_jobj;
        JOBJ_GetChild(hud_jobj, &text_jobj, 2 + i, -1);
        JOBJ_GetWorldPosition(text_jobj, 0, &text_pos);

        // adjust scale
        Vec3 *scale = &hud_jobj->scale;
        // text scale
        hud_text->scale.X = (scale->X * 0.01) * LCLTEXT_SCALE;
        hud_text->scale.Y = (scale->Y * 0.01) * LCLTEXT_SCALE;
        hud_text->aspect.X = 165;

        // text position
        hud_text->trans.X = text_pos.X + (scale->X / 4.0);
        hud_text->trans.Y = (text_pos.Y * -1) + (scale->Y / 4.0);

        // dummy text
        Text_AddSubtext(hud_text, 0, 0, "-");
    }

    // reset all bar colors
    JOBJ *timingbar_jobj;
    JOBJ_GetChild(hud_jobj, &timingbar_jobj, LCLJOBJ_BAR, -1); // get timing bar jobj
    DOBJ *d = timingbar_jobj->dobj;
    int count = 0;
    while (d != 0)
    {
        // if a box dobj
        if ((count >= 0) && (count < 30))
        {

            // if mobj exists (it will)
            MOBJ *m = d->mobj;
            if (m != 0)
            {

                HSD_Material *mat = m->mat;

                // set alpha
                mat->alpha = 0.7;

                // set color
                mat->diffuse = tmgbar_black;
            }
        }

        // inc
        count++;
        d = d->next;
    }

    return 0;
}
void Ledgedash_HUDThink(LedgedashData *event_data, FighterData *hmn_data)
{

    // run tip logic
    Tips_Think(event_data, hmn_data);

    JOBJ *hud_jobj = event_data->hud.gobj->hsd_object;

    // increment timer
    event_data->hud.timer++;

    // check to initialize timer
    if ((hmn_data->state_id == ASID_CLIFFWAIT) && (hmn_data->TM.state_frame == 1))
    {
        event_data->hud.timer = 0;
        event_data->hud.is_release = 0;
        event_data->hud.is_jump = 0;
        event_data->hud.is_airdodge = 0;
        event_data->hud.is_aerial = 0;
        event_data->hud.is_land = 0;
        event_data->hud.is_actionable = 0;

        // init action log
        for (int i = 0; i < sizeof(event_data->hud.action_log) / sizeof(u8); i++)
        {
            event_data->hud.action_log[i] = 0;
        }
    }

    int curr_frame = event_data->hud.timer;

    // update action log
    if (curr_frame < (sizeof(event_data->hud.action_log) / sizeof(u8)))
    {
        bp();

        // look for cliffwait
        if (hmn_data->state_id == ASID_CLIFFWAIT)
        {
            event_data->hud.is_release = 1;
            event_data->hud.action_log[curr_frame] = LDACT_CLIFFWAIT;
        }
        // look for release
        else if (hmn_data->state_id == ASID_FALL)
        {
            event_data->hud.is_release = 1;
            event_data->hud.action_log[curr_frame] = LDACT_FALL;
        }
        // look for jump
        else if ((hmn_data->state_id == ASID_JUMPAERIALF) || (hmn_data->state_id == ASID_JUMPAERIALB))
        {
            event_data->hud.is_jump = 1;
            event_data->hud.action_log[curr_frame] = LDACT_JUMP;
        }
        // look for airdodge
        else if (hmn_data->state_id == ASID_ESCAPEAIR)
        {
            //event_data->hud.is_airdodge = 1;
            event_data->hud.action_log[curr_frame] = LDACT_AIRDODGE;
        }
        // look for aerial
        else if (hmn_data->attack_kind != 1)
        {
            event_data->hud.is_aerial = 1;
            event_data->hud.action_log[curr_frame] = LDACT_ATTACK;
        }
        // look for land
        else if ((hmn_data->state_id == ASID_LANDING) || (hmn_data->state_id == ASID_LANDINGFALLSPECIAL))
        {
            event_data->hud.is_land = 1;
            event_data->hud.action_log[curr_frame] = LDACT_LANDING;
        }
    }

    // grab airdodge angle
    if (event_data->hud.is_airdodge == 0)
    {

        if ((hmn_data->state_id == ASID_ESCAPEAIR) || (hmn_data->TM.state_prev[0] == ASID_ESCAPEAIR))
        {
            // determine airdodge angle
            float angle = atan2(hmn_data->input.lstick_y, hmn_data->input.lstick_x) - -(M_PI / 2);

            // save airdodge angle
            event_data->hud.airdodge_angle = angle;
            event_data->hud.is_airdodge = 1;
        }
    }

    // look for actionable
    if ((event_data->hud.is_actionable == 0) &&
        ((((hmn_data->state_id == ASID_WAIT) || (hmn_data->TM.state_prev[0] == ASID_WAIT)) && (hmn_data->TM.state_frame <= 1)) || // prev frame too cause you can attack on the same frame
         ((hmn_data->state_id == ASID_LANDING) && (hmn_data->TM.state_frame >= hmn_data->attr.normal_landing_lag)) ||
         ((hmn_data->TM.state_prev[0] == ASID_LANDING) && (hmn_data->TM.state_prev_frames[0] >= hmn_data->attr.normal_landing_lag))))
    {
        event_data->hud.is_actionable = 1;
        event_data->hud.actionable_frame = event_data->hud.timer;

        SFX_PlayRaw(303, 255, 128, 20, 3);

        // reset all bar colors
        JOBJ *timingbar_jobj;
        JOBJ_GetChild(hud_jobj, &timingbar_jobj, LCLJOBJ_BAR, -1); // get timing bar jobj

        // update bar colors
        DOBJ *d = timingbar_jobj->dobj;
        int count = 0;
        while (d != 0)
        {
            // if a box dobj
            if ((count >= 0) && (count < 30))
            {

                // if mobj exists (it will)
                MOBJ *m = d->mobj;
                if (m != 0)
                {

                    HSD_Material *mat = m->mat;
                    int this_frame = 29 - count;
                    GXColor *bar_color;

                    // check if GALINT frame
                    if ((this_frame >= curr_frame) && ((this_frame <= (curr_frame + hmn_data->hurtstatus.ledge_intang_left))))
                        bar_color = &tmgbar_blue;
                    else
                        bar_color = tmgbar_colors[event_data->hud.action_log[this_frame]];

                    mat->diffuse = *bar_color;
                }
            }

            // inc
            count++;
            d = d->next;
        }

        // output remaining airdodge angle
        if (event_data->hud.is_airdodge == 1)
            Text_SetText(event_data->hud.text_angle, 0, "%.2f", fabs(event_data->hud.airdodge_angle / M_1DEGREE));
        else
            Text_SetText(event_data->hud.text_angle, 0, "-");

        // output remaining GALINT
        Text *text_galint = event_data->hud.text_galint;
        if (hmn_data->hurtstatus.ledge_intang_left > 0)
        {
            static GXColor galint_green = {150, 255, 150, 255};
            Text_SetColor(text_galint, 0, &galint_green);
            Text_SetText(text_galint, 0, "%df", hmn_data->hurtstatus.ledge_intang_left);
        }
        else
        {
            Text_SetColor(text_galint, 0, &tmgbar_white);
            Text_SetText(text_galint, 0, "-");
        }
    }

    // update HUD anim
    JOBJ_AnimAll(hud_jobj);

    return;
}
void LedgedashHUDCamThink(GOBJ *gobj)
{

    // if HUD enabled and not paused
    if ((LdshOptions_Main[1].option_val == 0) && (Pause_CheckStatus(1) != 2))
    {
        CObjThink_Common(gobj);
    }

    return;
}

// Fighter fuctions
void Ledgedash_FtInit(LedgedashData *event_data)
{
    GOBJ *hmn = Fighter_GetGObj(0);
    FighterData *hmn_data = hmn->userdata;

    // search for nearest ledge
    float ledge_dir;
    int line_index = Ledge_Find(0, 0, &ledge_dir);
    if (line_index == -1)
        event_vars->Tip_Display(500 * 60, "Error:\nIt appears there are no \nledges on this stage...");
    else
        Fighter_PlaceOnLedge(event_data, hmn, line_index, ledge_dir);

    return;
}
void Ledgedash_FighterThink(LedgedashData *event_data, GOBJ *hmn)
{
    FighterData *hmn_data = hmn->userdata;

    if (hmn_data->input.down & (HSD_BUTTON_DPAD_LEFT | HSD_BUTTON_DPAD_RIGHT))
    {

        // get current ledge position
        CollVert *collvert = *stc_collvert;
        CollLine *collline = *stc_collline;
        CollVert *this_vert;
        CollLine *this_line = &collline[event_data->ledge_line];
        float ledge_pos;
        if (event_data->ledge_dir == -1)
            ledge_pos = collvert[this_line->info->vert_next].pos_curr.X;
        else if (event_data->ledge_dir == 1)
            ledge_pos = collvert[this_line->info->vert_prev].pos_curr.X;

        // find ledge
        float ledge_dir;
        int line_index = -1;
        if (hmn_data->input.down & HSD_BUTTON_DPAD_LEFT)
            line_index = Ledge_Find(-1, ledge_pos, &ledge_dir);
        else if (hmn_data->input.down & HSD_BUTTON_DPAD_RIGHT)
            line_index = Ledge_Find(1, ledge_pos, &ledge_dir);

        if (line_index != -1)
            Fighter_PlaceOnLedge(event_data, hmn, line_index, ledge_dir);
    }

    return;
}
int Ledge_Find(int search_dir, float xpos_start, float *ledge_dir)
{

    // get line and vert pointers
    CollLine *collline = *stc_collline;
    CollVert *collvert = *stc_collvert;
    CollDataStage *coll_data = *stc_colldata;

    // get initial closest
    float xpos_closest;
    if (search_dir == -1) // search left
        xpos_closest = -5000;
    else if (search_dir == 1) // search right
        xpos_closest = 5000;
    else // search both
        xpos_closest = 5000;

    // look for the closest ledge
    CollLine *line_closest = 0;
    int index_closest = -1;
    int group_index = 0;                  // first ground link
    int group_num = coll_data->group_num; // ground link num
    CollGroup *this_group = *stc_firstcollgroup;
    while (this_group != 0) // loop through ground links
    {

        // 2 passes, one for ground and one for dynamic lines
        int line_index, line_num;
        for (int i = 0; i < 2; i++)
        {
            // first pass, use floors
            if (i == 0)
            {
                line_index = this_group->desc->floor_start;          // first ground link
                line_num = line_index + this_group->desc->floor_num; // ground link num
            }
            // second pass, use dynamics
            else if (i == 1)
            {
                line_index = this_group->desc->dyn_start;          // first ground link
                line_num = line_index + this_group->desc->dyn_num; // ground link num
            }

            // loop through lines
            while (line_index < line_num)
            {
                // get all data for this line
                CollLine *this_line = &collline[line_index]; // ??? i actually dont know why i cant access this directly
                CollLineInfo *this_lineinfo = this_line->info;

                // check if this link is a ledge
                if (this_lineinfo->is_ledge)
                {

                    // check both sides of this ledge
                    Vec3 ledge_pos;
                    for (int j = 0; j < 2; j++)
                    {
                        // first pass, check left
                        if (j == 0)
                        {
                            GrColl_GetLedgeLeft2(line_index, &ledge_pos);
                        }
                        else if (j == 1)
                        {
                            GrColl_GetLedgeRight2(line_index, &ledge_pos);
                        }

                        // is within the camera range
                        if ((ledge_pos.X > Stage_GetCameraLeft()) && (ledge_pos.X < Stage_GetCameraRight()) && (ledge_pos.Y > Stage_GetCameraBottom()) && (ledge_pos.Y < Stage_GetCameraTop()))
                        {

                            // check for any obstructions
                            float dir_mult;
                            if (j == 0) // left ledge
                                dir_mult = -1;
                            else if (j == 1) // right ledge
                                dir_mult = 1;
                            int ray_index;
                            int ray_kind;
                            Vec2 ray_angle;
                            Vec3 ray_pos;
                            float from_x = ledge_pos.X + (2 * dir_mult);
                            float to_x = from_x;
                            float from_y = ledge_pos.Y + 5;
                            float to_y = from_y - 10;
                            int is_ground = Stage_RaycastGround(&ray_pos, &ray_index, &ray_kind, &ray_angle, -1, -1, -1, 0, from_x, from_y, to_x, to_y, 0);
                            if (is_ground == 0)
                            {
                                int is_closer = 0;

                                if (search_dir == -1) // check if to the left
                                {
                                    if ((ledge_pos.X > xpos_closest) && (ledge_pos.X < xpos_start))
                                        is_closer = 1;
                                }
                                else if (search_dir == 1) // check if to the right
                                {
                                    if ((ledge_pos.X < xpos_closest) && (ledge_pos.X > xpos_start))
                                        is_closer = 1;
                                }
                                else // check if any direction
                                {
                                    float dist_old = fabs(xpos_start - xpos_closest);
                                    float dist_new = fabs(xpos_start - ledge_pos.X);
                                    if (dist_new < dist_old)
                                        is_closer = 1;
                                }

                                // determine direction
                                if (is_closer)
                                {

                                    // now determine if this line is a ledge in this direction
                                    if (j == 0) // left ledge
                                    {
                                        CollLine *prev_line = &collline[this_lineinfo->line_prev];          // ??? i actually dont know why i cant access this directly
                                        if ((this_lineinfo->line_prev == -1) || (prev_line->is_rwall == 1)) // if prev line is a right wall / if prev line doesnt exist
                                        {

                                            // save info on this line
                                            xpos_closest = ledge_pos.X; // save left vert's X position
                                            line_closest = this_line;
                                            index_closest = line_index;
                                            *ledge_dir = 1;
                                        }
                                    }
                                    else if (j == 1) // right ledge
                                    {
                                        CollLine *next_line = &collline[this_lineinfo->line_next];          // ??? i actually dont know why i cant access this directly
                                        if ((this_lineinfo->line_prev == -1) || (next_line->is_lwall == 1)) // if prev line is a right wall / if prev line doesnt exist
                                        {

                                            // save info on this line
                                            xpos_closest = ledge_pos.X; // save left vert's X position
                                            line_closest = this_line;
                                            index_closest = line_index;
                                            *ledge_dir = -1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                line_index++;
            }
        }

        // get next
        this_group = this_group->next;
    }

    return index_closest;
}
void Fighter_PlaceOnLedge(LedgedashData *event_data, GOBJ *hmn, int line_index, float ledge_dir)
{
    FighterData *hmn_data = hmn->userdata;

    // save ledge position
    Vec3 ledge_pos;
    event_data->ledge_line = line_index;
    event_data->ledge_dir = ledge_dir;

    // place player on this ledge
    FtCliffCatch *ft_state = &hmn_data->state_var;
    hmn_data->facing_direction = ledge_dir;
    ft_state->ledge_index = line_index; // store line index
    Fighter_EnterCliffWait(hmn);
    ft_state->timer = 1; // spoof as on ledge for a frame already
    Fighter_LoseGroundJump(hmn_data);
    Fighter_EnableCollUpdate(hmn_data);
    Fighter_MoveToCliff(hmn);
    Fighter_UpdatePosition(hmn);
    Coll_CheckLedge(&hmn_data->collData);
    Fighter_UpdateCamera(hmn);
    hmn_data->phys.selfVel.X = 0;
    hmn_data->phys.selfVel.Y = 0;
    ftCommonData *ftcommon = *stc_ftcommon;
    Fighter_ApplyIntang(hmn, ftcommon->cliff_invuln_time);

    return;
}
void Fighter_UpdatePosition(GOBJ *fighter)
{

    FighterData *fighter_data = fighter->userdata;

    // Update Position (Copy Physics XYZ into all ECB XYZ)
    fighter_data->collData.topN_Curr.X = fighter_data->phys.pos.X;
    fighter_data->collData.topN_Curr.Y = fighter_data->phys.pos.Y;
    fighter_data->collData.topN_Prev.X = fighter_data->phys.pos.X;
    fighter_data->collData.topN_Prev.Y = fighter_data->phys.pos.Y;
    fighter_data->collData.topN_CurrCorrect.X = fighter_data->phys.pos.X;
    fighter_data->collData.topN_CurrCorrect.Y = fighter_data->phys.pos.Y;
    fighter_data->collData.topN_Proj.X = fighter_data->phys.pos.X;
    fighter_data->collData.topN_Proj.Y = fighter_data->phys.pos.Y;

    // Update Collision Frame ID
    fighter_data->collData.coll_test = *stc_colltest;

    // Adjust JObj position (code copied from 8006c324)
    JOBJ *fighter_jobj = fighter->hsd_object;
    fighter_jobj->trans.X = fighter_data->phys.pos.X;
    fighter_jobj->trans.Y = fighter_data->phys.pos.Y;
    fighter_jobj->trans.Z = fighter_data->phys.pos.Z;
    JOBJ_SetMtxDirtySub(fighter_jobj);

    // Update Static Player Block Coords
    Fighter_SetPosition(fighter_data->ply, fighter_data->flags.ms, &fighter_data->phys.pos);
    return;
}
void Fighter_UpdateCamera(GOBJ *fighter)
{
    FighterData *fighter_data = fighter->userdata;

    // Update camerabox pos
    Fighter_UpdateCameraBox(fighter);

    // Update tween
    fighter_data->cameraBox->boundleft_curr = fighter_data->cameraBox->boundleft_proj;
    fighter_data->cameraBox->boundright_curr = fighter_data->cameraBox->boundright_proj;

    Match_CorrectCamera();
}

// Tips Functions
void Tips_Think(LedgedashData *event_data, FighterData *hmn_data)
{

    return;
}

// Initial Menu
static EventMenu *Event_Menu = &LdshMenu_Main;
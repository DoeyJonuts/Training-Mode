void Menu_Confirm_Think(GOBJ *menu_gobj)
{

    // init
    int down = Pad_GetRapidHeld(*stc_css_hmnport);

    // first ensure memcard is still inserted
    s32 memSize, sectorSize;
    if (CARDProbeEx(import_data.memcard_slot, &memSize, &sectorSize) != CARD_RESULT_READY)
        goto EXIT;

    switch (import_data.confirm.kind)
    {
    case (CFRM_LOAD):
    {
        // cursor movement
        if (down & (HSD_BUTTON_RIGHT | HSD_BUTTON_DPAD_RIGHT)) // check for cursor right
        {
            if (import_data.confirm.cursor < 1)
            {
                import_data.confirm.cursor++;
                SFX_PlayCommon(2);
            }
        }
        else if (down & (HSD_BUTTON_LEFT | HSD_BUTTON_DPAD_LEFT)) // check for cursor down
        {
            if (import_data.confirm.cursor > 0)
            {
                import_data.confirm.cursor--;
                SFX_PlayCommon(2);
            }
        }

        // highlight cursor
        int cursor = import_data.confirm.cursor;
        for (int i = 0; i < 2; i++)
        {
            Text_SetColor(import_data.confirm.text, i + 1, &text_white);
        }
        Text_SetColor(import_data.confirm.text, cursor + 1, &text_gold);

        // check for exit
        if (down & HSD_BUTTON_B)
        {
        EXIT:
            Menu_Confirm_Exit(menu_gobj);
            SFX_PlayCommon(0);
            import_data.menu_state = IMP_SELFILE;
        }

        // check for select
        else if (down & HSD_BUTTON_A)
        {
            // check which option is selected
            if (cursor == 0)
            {

                // get variables and junk
                VSMinorData *css_minorscene = *stc_css_minorscene;
                int this_file_index = (import_data.page * IMPORT_FILESPERPAGE) + import_data.cursor;
                ExportHeader *header = &import_data.header[import_data.cursor];
                Preload *preload = Preload_GetTable();

                // get match data
                u8 hmn_kind = header->metadata.hmn;
                u8 hmn_costume = header->metadata.hmn_costume;
                u8 cpu_kind = header->metadata.cpu;
                u8 cpu_costume = header->metadata.cpu_costume;
                u16 stage_kind = header->metadata.stage_external;

                // determine which player index for hmn and cpu
                u8 hmn_index, cpu_index;
                if (*stc_css_hmnport > 0)
                {
                    cpu_index = 0;
                    hmn_index = 1;
                }
                else
                {
                    hmn_index = 0;
                    cpu_index = 1;
                }

                // set fighters
                css_minorscene->vs_data.match_init.playerData[hmn_index].kind = hmn_kind;
                css_minorscene->vs_data.match_init.playerData[hmn_index].costume = hmn_costume; // header->metadata.hmn_costume;
                preload->fighters[hmn_index].kind = hmn_kind;
                preload->fighters[hmn_index].costume = hmn_costume;
                css_minorscene->vs_data.match_init.playerData[cpu_index].kind = cpu_kind;
                css_minorscene->vs_data.match_init.playerData[cpu_index].costume = cpu_costume; // header->metadata.cpu_costume;
                preload->fighters[cpu_index].kind = cpu_kind;
                preload->fighters[cpu_index].costume = cpu_costume;

                // set stage
                css_minorscene->vs_data.match_init.stage = stage_kind;
                preload->stage = stage_kind;

                // load files
                Preload_Update();

                // advance scene
                *stc_css_exitkind = 1;

                // HUGE HACK ALERT
                EventDesc *(*GetEventDesc)(int page, int event) = RTOC_PTR(TM_DATA + (24 * 4));
                EventDesc *event_desc = GetEventDesc(1, 0);
                event_desc->isSelectStage = 0;
                event_desc->matchData->stage = stage_kind;
                *onload_fileno = this_file_index;
                *onload_slot = import_data.memcard_slot;

                SFX_PlayCommon(1);
            }
            else
                goto EXIT;
        }

        break;
    }
    case (CFRM_OLD):
    {
        // check for select
        if (down & (HSD_BUTTON_A | HSD_BUTTON_B))
        {
            Menu_Confirm_Exit(menu_gobj);
            SFX_PlayCommon(0);
            import_data.menu_state = IMP_SELFILE;
        }
        break;
    }
    case (CFRM_NEW):
    {
        // check for select
        if (down & (HSD_BUTTON_A | HSD_BUTTON_B))
        {
            Menu_Confirm_Exit(menu_gobj);
            SFX_PlayCommon(0);
            import_data.menu_state = IMP_SELFILE;
        }
        break;
    }
    case (CFRM_DEL):
    {

        // cursor movement
        if (down & (HSD_BUTTON_RIGHT | HSD_BUTTON_DPAD_RIGHT)) // check for cursor right
        {
            if (import_data.confirm.cursor < 1)
            {
                import_data.confirm.cursor++;
                SFX_PlayCommon(2);
            }
        }
        else if (down & (HSD_BUTTON_LEFT | HSD_BUTTON_DPAD_LEFT)) // check for cursor down
        {
            if (import_data.confirm.cursor > 0)
            {
                import_data.confirm.cursor--;
                SFX_PlayCommon(2);
            }
        }

        // highlight cursor
        int cursor = import_data.confirm.cursor;
        for (int i = 0; i < 2; i++)
        {
            Text_SetColor(import_data.confirm.text, i + 1, &text_white);
        }
        Text_SetColor(import_data.confirm.text, cursor + 1, &text_gold);

        // check for back
        if (down & HSD_BUTTON_B)
        {
        RETURN_TO_FILESEL:
            Menu_Confirm_Exit(menu_gobj);
            SFX_PlayCommon(0);
            import_data.menu_state = IMP_SELFILE;
        }

        // check for confirm
        else if (down & HSD_BUTTON_A)
        {

            if (cursor == 0)
            {
                SFX_PlayCommon(1);

                // delete selected recording
                int this_file_index = (import_data.page * IMPORT_FILESPERPAGE) + import_data.cursor;
                Menu_SelFile_DeleteFile(menu_gobj, this_file_index);

                // close dialog
                Menu_Confirm_Exit(menu_gobj);

                // reload selfile
                Menu_SelFile_Exit(menu_gobj); // close select file
                Menu_SelFile_Init(menu_gobj); // open select file
            }
            else
                goto RETURN_TO_FILESEL;
        }
        break;
    }
    case (CFRM_ERR):
    {

        // cursor movement
        if (down & (HSD_BUTTON_RIGHT | HSD_BUTTON_DPAD_RIGHT)) // check for cursor right
        {
            if (import_data.confirm.cursor < 1)
            {
                import_data.confirm.cursor++;
                SFX_PlayCommon(2);
            }
        }
        else if (down & (HSD_BUTTON_LEFT | HSD_BUTTON_DPAD_LEFT)) // check for cursor down
        {
            if (import_data.confirm.cursor > 0)
            {
                import_data.confirm.cursor--;
                SFX_PlayCommon(2);
            }
        }

        // highlight cursor
        int cursor = import_data.confirm.cursor;
        for (int i = 0; i < 2; i++)
        {
            Text_SetColor(import_data.confirm.text, i + 2, &text_white);
        }
        Text_SetColor(import_data.confirm.text, cursor + 2, &text_gold);

        // check for back
        if (down & HSD_BUTTON_B)
        {
        NO_DELETE_CORRUPT:
            Menu_Confirm_Exit(menu_gobj); // close dialog
            Menu_SelFile_Exit(menu_gobj); // close select file
            Menu_SelCard_Init(menu_gobj); // open select card
            SFX_PlayCommon(0);
            //import_data.menu_state = IMP_SELCARD;
        }

        // check for confirm
        else if (down & HSD_BUTTON_A)
        {

            if (cursor == 0)
            {
                SFX_PlayCommon(1);

                // delete bad recordings
                Menu_SelFile_DeleteUnsupported(menu_gobj);

                // close dialog
                Menu_Confirm_Exit(menu_gobj);

                // reload selfile
                Menu_SelFile_Exit(menu_gobj); // close select file
                Menu_SelFile_Init(menu_gobj); // open select file
            }
            else
                goto NO_DELETE_CORRUPT;
        }
        break;
    }
    }

    return;
}

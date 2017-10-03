
void
process_line(char *line, int len, char *line_wo_date, int len_wo_date)
{
    struct rule * rp;
    regmatch_t matches[ 25 ];
    int regerr, matched;
    char errstring[100];

    line_wo_date[len_wo_date]=0;

#if DEBUG_LOG
    fprintf(debug_log_file, "input line (%d): '", len_wo_date);
    fwrite(line_wo_date, len_wo_date, 1, debug_log_file);
    fprintf(debug_log_file, "'\n");
#endif

    if (raw_output_file)
    {
        fprintf(raw_output_file,"%s\n", line);
        fflush(raw_output_file);
    }
    matched = 0;
    for (rp = rules; rp; rp = rp->next)
    {
        switch (rp->action)
        {
        case ACTION_COMPARE_IGNORE:
        case ACTION_COMPARE_STORE:
            regerr = regexec( &rp->expr, line_wo_date, 25, matches, 0 );
            if (regerr != REG_NOERROR && regerr != REG_NOMATCH)
            {
                regerror(regerr, &rp->expr, errstring, sizeof(errstring));
                fprintf(stderr, "pattern matching input: %d: %s\n",
                        regerr, errstring);
                return;
            }
            if ( regerr == REG_NOERROR && (int)matches[0].rm_so != -1 )
                matched = 1;
            break;

        case ACTION_DEFAULT_STORE:
            matched = 1;
            break;
        }
        if (matched)
            break;
    }
#if DEBUG_LOG
    fprintf(debug_log_file, "matched rule %d: ", rp->rule_id);
    {
        int i = 0;
        for (i = 0; i < 5; i++)
            fprintf(debug_log_file, "(%d,%d) ", matches[i].rm_so, matches[i].rm_eo);
        fprintf(debug_log_file, "\n");
    }
    fflush(debug_log_file);
#endif
    if (!matched)
    {
        // default store should always be last in the rule list, so
        // this should not happen.
        fprintf(stderr, "internal error, didn't match anything!\n");
        exit(1);
    }
    switch (rp->action)
    {
    case ACTION_COMPARE_STORE:
    case ACTION_DEFAULT_STORE:
        fwrite(line, len, 1, rp->file->file);
        fprintf(rp->file->file, "\n");
        if (rp->file->enable_stdout)
        {
            fwrite(line_wo_date, len_wo_date, 1, stdout);
            fprintf(stdout, "\n");
        }
        break;
    }
}

void
flush_files(void)
{
    struct rule_file * rfp;
    for (rfp = rule_files; rfp; rfp = rfp->next)
        fflush(rfp->file);
}

void
close_files(void)
{
    struct rule_file * rfp;
    for (rfp = rule_files; rfp; rfp = rfp->next)
        fclose(rfp->file);
}

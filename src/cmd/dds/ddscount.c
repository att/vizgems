#pragma prototyped

#include <ast.h>
#include <option.h>
#include <cdt.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddscount (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddscount - compute sums]"
"[+DESCRIPTION?\bddscount\b reads records from a DDS data file (or standard"
" input) and for each unique combination of fields, it either counts how many"
" records there are, or sums up a specific field.]"
"[100:is?specifies the schema of the input DDS file."
" This flag overrides any schema specified in the input file."
" Can be used to \aimport\a a fixed record data file that has no DDS header."
"]:[schemafile]"
"[102:osm?specifies that the output file should contain the complete schema,"
" just the name of the schema, or no schema at all."
" When a data file contains just the schema name, DDS searches for a file with"
" that name in the user's path (pointed to by the PATH environment variable)."
" For every directory in the path, such as /home/ek/bin, the schema is searched"
" for in the corresponding lib/swift directory, such as /home/ek/lib/swift."
" The last option, none, can be used to \aexport\a a data file."
" The default mode is \bfull\b."
"]:[(full|name|none):oneof]{[+full][+name][+none]}"
"[103:oce?specifies that the output data should be written to one or more"
" pipelines."
" Each command in the argument is assumed to be startable using the user's"
" shell (pointed to by the SHELL environment variable)."
" All commands must be DDS commands, or if the commands are scripts, they must"
" use a DDS command to process the input data."
" Each command should be enclosed in quotes if it contains spaces."
" The entire option string should be enclosed in quotes (different than the"
" ones used per command)."
" The default is to write the data to the standard output."
"]:['cmd1 ...']"
"[104:ozm?specifies the compression mode for the output file."
" The mode can be a vcodex spec, or \bnone\b for no compression,"
" or \brdb/rtb\b for the vcodex \brdb/rtable\b codex,"
" using the DDS schema field sizes."
" The default mode is \bnone\b."
"]:['vcodex spec|none|rdb|rtb']"
"[210:ke?specifies the names of fields to be used as a key for counting."
"]:['field1 ...']"
"[260:ce?specifies the name of the field to be summed."
" The default action is to create and sum a new field."
" The initial value of the new field is \b1\b."
"]:['expression']"
"[262:cso?specifies the file containing the precompiled shared object for"
" counting."
"]:[file]"
"[313:tfp?specifies the name of a temporary file."
" When \bddscount\b exceeds the number of records specified by the \bmaxr\b"
" option, instead of inserting any more new records, it writes these new"
" records to the temporary file."
" Any records that match the keys of records already in memory continue to be"
" summed."
" When the input file is consumed, \bddscount\b writes the records it has in"
" memory to the output stream, and replaces the input stream with the"
" temporary file."
" It continues this process until there are no more records in the temporary"
" file."
" The default is to do all the work in memory."
"]:[file]"
"[314:maxr?specifies the maximum number of records to keep in memory."
" When \bddscount\b exceeds this number it behaves as described in the \btfp\b"
" option."
" The default number is \b1,000,000\b but it is not used unless either the"
" \btfp\b or the \bpc\b options are specified."
"]#[records]"
"[315:pc?specifies that it is acceptable to generate partial counts."
" When \bddscount\b exceeds the number of records specified by the \bmaxr\b"
" variable, it writes the in-memory records to the output stream, and starts"
" from scratch."
" The final output may contain duplicate key / count records."
" The default is to do the complete count."
"]"
"[400:ccflags?specifies extra flags for the compiler such as -I flags."
"]:[ccflags]"
"[401:ldflags?specifies extra flags for the compiler such as -L flags."
"]:[ldflags]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+EXAMPLES?Assuming the following 4 records:]{"
"[+]"
"[+f1 f2 f3\b]"
"[+--------\b]"
"[+10 11  5\b]"
"[+10 11 15\b]"
"[+10 21 25\b]"
"[+20 31 35\b]"
"}"
"[+?\bddscount -ke f1\b will output 2 records:]{"
"[+]"
"[+f1 f2 f3 count0\b]"
"[+---------------\b]"
"[+10 11  5 3\b]"
"[+20 31 35 1\b]"
"}"
"[+?while \bddscount -ke 'f1 f2' -ce f3\b will output 3 records:]{"
"[+]"
"[+f1 f2 f3\b]"
"[+--------\b]"
"[+10 11 20\b]"
"[+10 21 25\b]"
"[+20 31 35\b]"
"}"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

int main (int argc, char **argv) {
    int norun;
    char *schemaname;
    int osm;
    char *ozm;
    char *cmds, *kexpr, *cexpr, *csofile, *tmpfile;
    long long maxrecn, steprecn;
    int partialcount;
    char *ccflags, *ldflags;
    DDSschema_t *schemap;
    int fieldi;
    DDScounter_t *counterp;
    DDSheader_t hdr;
    char buf[10000];
    Sfio_t *ifp, *ofp;
    Sfio_t *tmpfp, *fp;
    int tmpcount, count, insertflag;
    Dt_t *dict;
    int offset;
    char *vp;
    int ret, ret2;

    schemaname = NULL;
    schemap = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    kexpr = cexpr = NULL, csofile = NULL;
    tmpfile = NULL, tmpfp = NULL;
    maxrecn = 1000000;
    partialcount = FALSE;
    cmds = NULL;
    ccflags = ldflags = "";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            schemaname = opt_info.arg;
            continue;
        case -102:
            if (strcmp (opt_info.arg, "none") == 0)
                osm = 0;
            else if (strcmp (opt_info.arg, "name") == 0)
                osm = DDS_HDR_NAME;
            else if (strcmp (opt_info.arg, "full") == 0)
                osm = DDS_HDR_NAME | DDS_HDR_DESC;
            continue;
        case -103:
            cmds = opt_info.arg;
            continue;
        case -104:
            ozm = opt_info.arg;
            continue;
        case -210:
            kexpr = opt_info.arg;
            continue;
        case -260:
            cexpr = opt_info.arg;
            continue;
        case -262:
            csofile = opt_info.arg;
            continue;
        case -313:
            tmpfile = opt_info.arg, partialcount = FALSE;
            continue;
        case -314:
            maxrecn = opt_info.num;
            continue;
        case -315:
            partialcount = TRUE, tmpfile = NULL;
            continue;
        case -400:
            ccflags = opt_info.arg;
            continue;
        case -401:
            ldflags = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "ddscount", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddscount", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc > 1)
        SUerror ("ddscount", "too many parameters");
    DDSinit ();
    ifp = sfstdin;
    if (argc == 1 && !(ifp = sfopen (NULL, argv[0], "r")))
        SUerror ("ddscount", "sfopen failed for %s", argv[0]);
    steprecn = maxrecn / 10;
    if (!(ifp = SUsetupinput (ifp, 1048576)))
        SUerror ("ddscount", "setupinput failed");
    if (!(ofp = SUsetupoutput (cmds, sfstdout, 1048576)))
        SUerror ("ddscount", "setupoutput failed");
    if ((ret = DDSreadheader (ifp, &hdr)) == -1)
        SUerror ("ddscount", "DDSreadheader failed");
    if (ret == -2)
        goto done;
    if (ret == 0) {
        if (hdr.schemap)
            schemap = hdr.schemap;
        if (hdr.schemaname)
            schemaname = hdr.schemaname;
    }
    if (!schemap) {
        if (!schemaname)
            SUerror ("ddscount", "no schema specified");
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddscount", "DDSloadschema failed");
    }
    if ((!kexpr || strcmp (kexpr, "all") == 0) && !csofile) {
        for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
            if (cexpr && strcmp (cexpr, schemap->fields[fieldi].name) == 0)
                continue;
            if (fieldi > 0)
                strcat (buf, " ");
            strcat (buf, schemap->fields[fieldi].name);
        }
        kexpr = buf;
    }
    if (!csofile) {
        if (!(counterp = DDScreatecounter (
            schemap, kexpr, cexpr, ccflags, ldflags
        )))
            SUerror ("ddscount", "DDScreatecounter failed");
    } else {
        if (!(counterp = DDSloadcounter (schemap, csofile)))
            SUerror ("ddscount", "DDSloadcounter failed");
    }
    hdr.schemaname = counterp->schemap->name, hdr.schemap = counterp->schemap;
    hdr.contents = osm;
    hdr.vczspec = ozm;
    if (osm && DDSwriteheader (ofp, &hdr) == -1)
        SUerror ("ddscount", "DDSwriteheader failed");
    if (tmpfile) {
        if (!(tmpfp = sfopen (NULL, tmpfile, "w+")))
            SUerror ("ddscount", "sfopen failed");
        if (!(tmpfp = SUsetupoutput (NULL, tmpfp, 1048576)))
            SUerror ("ddscount", "setupoutput failed");
    }
    fp = ifp;
    insertflag = TRUE;
    count = 0;

again:
    tmpcount = 0;
    if ((*counterp->init) (&dict, &offset) == -1)
        SUerror ("ddscount", "counterp->init failed");
    while ((ret = DDSreadrecord (
        fp, schemap->recbuf, schemap
    )) == schemap->recsize) {
        if ((ret2 = (*counterp->work) (
            schemap->recbuf, schemap->recsize, insertflag
        )) == 2) {
            if (DDSwriterecord (
                tmpfp, schemap->recbuf, schemap
            ) != schemap->recsize)
                SUerror ("ddscount", "incomplete write to tmp file");
            tmpcount++;
        } else if (ret2 == -1) {
            SUerror ("ddscount", "counting failed");
        }
        count++;
        if ((partialcount || tmpfp) && count % steprecn == 0 && insertflag)
            if (dtsize (dict) > maxrecn) {
                insertflag = FALSE;
                if (partialcount)
                    break;
            }
    }
    for (vp = dtfirst (dict); vp; vp = dtnext (dict, vp)) {
        if (DDSwriterecord (
            ofp, &vp[offset], counterp->schemap
        ) != counterp->schemap->recsize)
            SUerror ("ddscount", "incomplete write");
    }
    if (partialcount && !insertflag) {
        if ((*counterp->term) () == -1)
            SUerror ("ddscount", "counterp->term failed");
        insertflag = TRUE;
        goto again;
    }
    if (tmpcount > 0) {
        if (fp != ifp)
            sfclose (fp);
        if (rename (tmpfile, sfprints ("%s.old", tmpfile)) == -1)
            SUerror ("ddscount", "rename failed");
        fp = tmpfp;
        if (sfseek (fp, 0, SEEK_SET) == -1)
            SUerror ("ddscount", "sfseek failed");
        if (!(tmpfp = sfopen (NULL, tmpfile, "w+")))
            SUerror ("ddscount", "sfopen failed");
        if (!(tmpfp = SUsetupinput (tmpfp, 1048576)))
            SUerror ("ddscount", "setupinput failed");
        if ((*counterp->term) () == -1)
            SUerror ("ddscount", "counterp->term failed");
        insertflag = TRUE;
        goto again;
    }
    if (tmpfp) {
        if (fp != ifp)
            sfclose (fp), unlink (sfprints ("%s.old", tmpfile));
        sfclose (tmpfp), unlink (tmpfile);
    }
    if (ret != 0)
        SUerror ("ddscount", "incomplete read");
    if ((*counterp->term) () == -1)
        SUerror ("ddscount", "counterp->term failed");
    if (sfsync (NULL) < 0)
        SUerror ("ddscount", "sync failed");
    if (ofp != sfstdout)
        sfclose (ofp);
    DDSdestroycounter (counterp);

done:
    DDSterm ();
    return 0;
}

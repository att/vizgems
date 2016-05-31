
#include <ast.h>
#include <stdio.h>
#include <swift.h>
#include "vg_dq_vt_util.h"
#include "vg_dq_vt_util_cm.h"

int RImaxtsflag;

static Dtdisc_t colordisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static RIfont_t fonts[RI_FONT_SIZE];
static char *fontmap[][2] = {
    { "Arial",      "lib/fonts/LiberationSans-Regular.ttf"  },
    { "Verdana",    "lib/fonts/LiberationSans-Regular.ttf"  },
    { "Helvetica",  "lib/fonts/LiberationSans-Regular.ttf"  },
    { "sans-serif", "lib/fonts/LiberationSans-Regular.ttf"  },
    { "Times",      "lib/fonts/LiberationSerif-Regular.ttf" },
    { "Courier",    "lib/fonts/LiberationMono-Regular.ttf"  },
    { NULL,         NULL                                    }
};

static Dtdisc_t cssdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static int cssid;

static char *attrstr;
static int attrlen;

static char *fileprefix, *imageprefix;
static int maxpix;
static int imageid;

static Sfio_t *emfp;
static Sfio_t *strfp;

static char *tokb, *toke, *tokp;
#define TOKINIT(s) { \
    tokb = (s); \
    toke = tokp = NULL; \
}
#define NTOK { \
    if (!tokb) \
        tokp = tokb = toke = NULL; \
    else if ((toke = strchr (tokb, ' '))) \
        *toke = 0, tokp = tokb, tokb = toke + 1; \
    else \
        tokp = tokb, tokb = NULL; \
}
#define NTOKL(l) { \
    if (!tokb) \
        tokp = tokb = toke = NULL; \
    else if (strlen (tokb) >= (l)) \
        toke = tokb + (l), *toke = 0, tokp = tokb, tokb = toke + 1; \
    else \
        tokp = tokb = NULL; \
}

#define X(x) ((int) ((x) * 1.35))
#define Y(y) ((int) (rip->h - ((y) * 1.35)))
#define S(l) ((int) ((l) * 1.35))
#define B2E_CONST .55228474983079339840

static int parsecolor (RI_t *, char *);
static int findfont (char *, int, RIfont_t *);
static int gensplines (RI_t *, RIpoint_t *, int);
static char *htmlreenc (char *);
static char *nlreenc (char *);

int RIinit (char *fprefix, char *iprefix, int mp) {
    RImaxtsflag = TRUE;

    fonts[RI_FONT_0].size = 4, fonts[RI_FONT_0].font = gdfont4;
    fonts[RI_FONT_1].size = 5, fonts[RI_FONT_1].font = gdfont5;
    fonts[RI_FONT_2].size = 6, fonts[RI_FONT_2].font = gdfont6;
    fonts[RI_FONT_3].size = 7, fonts[RI_FONT_3].font = gdfont7;
    fonts[RI_FONT_4].size = 8, fonts[RI_FONT_4].font = gdfont8;
    fonts[RI_FONT_5].size = 9, fonts[RI_FONT_5].font = gdfont9;
    fonts[RI_FONT_6].size = 10, fonts[RI_FONT_6].font = gdfont10;
    fonts[RI_FONT_7].size = 11, fonts[RI_FONT_7].font = gdfont11;
    fonts[RI_FONT_8].size = 12, fonts[RI_FONT_8].font = gdfont12;
    fonts[RI_FONT_9].size = 13, fonts[RI_FONT_9].font = gdfont13;
    fonts[RI_FONT_10].size = 14, fonts[RI_FONT_10].font = gdfont14;
    fonts[RI_FONT_11].size = 15, fonts[RI_FONT_11].font = gdfont15;
    fonts[RI_FONT_12].size = 16, fonts[RI_FONT_12].font = gdfont16;
    fonts[RI_FONT_13].size = 17, fonts[RI_FONT_13].font = gdfont17;
    fonts[RI_FONT_14].size = 18, fonts[RI_FONT_14].font = gdfont18;
    fonts[RI_FONT_15].size = 19, fonts[RI_FONT_15].font = gdfont19;
    fonts[RI_FONT_16].size = 20, fonts[RI_FONT_16].font = gdfont20;
    if (!(emfp = sfopen (NULL, sfprints ("%s.embed", fprefix), "w"))) {
        SUwarning (0, "RIinit", "cannot create file %s.embed", fprefix);
        return -1;
    }
    if (!(fileprefix = vmstrdup (Vmheap, fprefix))) {
        SUwarning (0, "RIinit", "cannot copy fileprefix");
        return -1;
    }
    if (!(imageprefix = vmstrdup (Vmheap, iprefix))) {
        SUwarning (0, "RIinit", "cannot copy imageprefix");
        return -1;
    }
    maxpix = mp;
    strfp = sfstropen ();
    return 0;
}

static RI_t *RIopenimage (RI_t *);
static RI_t *RIopencanvas (RI_t *);
static RI_t *RIopentable (RI_t *);

RI_t *RIopen (int type, char *fprefix, int findex, int w, int h) {
    Vmalloc_t *vm;
    RI_t *rip;

    if (!(vm = vmopen (Vmdcheap, Vmbest, 0))) {
        SUwarning (0, "RIopen", "cannot create vm arena");
        return NULL;
    }
    if (!(rip = vmalloc (vm, sizeof (RI_t)))) {
        SUwarning (0, "RIopen", "cannot allocate ri");
        return NULL;
    }
    memset (rip, 0, sizeof (RI_t));
    rip->type = type;
    rip->vm = vm;
    if (!(rip->fprefix = vmstrdup (vm, fprefix))) {
        SUwarning (0, "RIopen", "cannot copy file name");
        return NULL;
    }
    rip->findex = findex;
    rip->w = w, rip->h = h;
    rip->ops = NULL, rip->opn = rip->opm = 0, rip->cflag = FALSE;
    rip->points = NULL, rip->pointn = rip->pointm = 0;
    rip->sppoints = NULL, rip->sppointn = rip->sppointm = 0;
    rip->areas = NULL, rip->arean = rip->aream = 0;
    if (!(rip->tocfp = sfopen (NULL, sfprints (
        "%s.%d.toc", fprefix, findex
    ), "w"))) {
        SUwarning (
            0, "RIopen", "cannot create file %s.%d.toc", fprefix, findex
        );
        return NULL;
    }
    sfprintf (sfstdout, "%s.%d.toc\n", fprefix, findex);
    if (!(rip->cdict = dtopen (&colordisc, Dtset))) {
        SUwarning (0, "RIopen", "cannot create colordict");
        return NULL;
    }

    switch (type) {
    case RI_TYPE_IMAGE:
        return RIopenimage (rip);
    case RI_TYPE_CANVAS:
        return RIopencanvas (rip);
    case RI_TYPE_TABLE:
        return RIopentable (rip);
    default:
        SUwarning (0, "RIopen", "unknown RI type %d", type);
        return NULL;
    }
    return NULL;
}

static RI_t *RIopenimage (RI_t *rip) {
    rip->u.image.slicen = (rip->w * rip->h + maxpix - 1) / maxpix;
    if (!(rip->u.image.img = gdImageCreateTrueColor (rip->w, rip->h))) {
        SUwarning (
            0, "RIopenimage", "cannot create image: %dx%d", rip->w, rip->h
        );
        return NULL;
    }
    rip->u.image.vpx = 0, rip->u.image.vpy = 0;
    rip->u.image.vpw = rip->w, rip->u.image.vph = rip->h;
    gdImageSetThickness (rip->u.image.img, 1);
    rip->u.image.bgid = gdImageColorAllocate (rip->u.image.img, 255, 255, 255);
    rip->u.image.fgid = gdImageColorAllocate (rip->u.image.img, 0, 0, 0);
    return rip;
}

static RI_t *RIopencanvas (RI_t *rip) {
    if (!(rip->u.canvas.fp = sfopen (NULL, sfprints (
        "%s.%d.draw.js", rip->fprefix, rip->findex
    ), "w"))) {
        SUwarning (
            0, "RIopencanvas", "cannot create file %s.%d.draw.js",
            rip->fprefix, rip->findex
        );
        return NULL;
    }
    sfprintf (sfstdout, "%s.%d.js\n", rip->fprefix, rip->findex);
    rip->u.canvas.vpx = 0, rip->u.canvas.vpy = 0;
    rip->u.canvas.vpw = rip->w, rip->u.canvas.vph = rip->h;
    sfprintf (
        rip->u.canvas.fp, "function %s_%d_draw () {\n",
        rip->fprefix, rip->findex
    );
    sfprintf (rip->u.canvas.fp, "  cntx.lineWidth = 1\n");
    return rip;
}

static RI_t *RIopentable (RI_t *rip) {
    if (!(rip->u.table.fp = sfopen (NULL, sfprints (
        "%s.%d.html", rip->fprefix, rip->findex
    ), "w"))) {
        SUwarning (
            0, "RIopentable", "cannot create file %s.%d.html",
            rip->fprefix, rip->findex
        );
        return NULL;
    }
    sfprintf (sfstdout, "%s.%d.html\n", rip->fprefix, rip->findex);
    if (!(rip->u.table.cssdict = dtopen (&cssdisc, Dtset))) {
        SUwarning (0, "RIopentable", "cannot create cssdict");
        return NULL;
    }
    sfprintf (sfstdout, "%s.%d.css\n", rip->fprefix, rip->findex);
    return rip;
}

static int RIcloseimage (RI_t *);
static int RIclosecanvas (RI_t *);
static int RIclosetable (RI_t *);

int RIclose (RI_t *rip) {
    int ret;

    switch (rip->type) {
    case RI_TYPE_IMAGE:
        ret = RIcloseimage (rip);
        break;
    case RI_TYPE_CANVAS:
        ret = RIclosecanvas (rip);
        break;
    case RI_TYPE_TABLE:
        ret = RIclosetable (rip);
        break;
    default:
        SUwarning (0, "RIclose", "unknown RI type %d", rip->type);
        return -1;
    }
    sfclose (rip->tocfp);
    dtclose (rip->cdict);
    vmclose (rip->vm);
    return ret;
}

static int RIcloseimage (RI_t *rip) {
    FILE *imgfp;
    Sfio_t *imfp, *infofp;
    RIarea_t *ap;
    int ai;
    int cpass, done, h1, h2, slicei, y1, y2;
    gdImage *img;

    if (rip->u.image.slicen == 1) {
        if (!(imgfp = fopen (sfprints (
            "%s.%d.gif", rip->fprefix, rip->findex
        ), "w"))) {
            SUwarning (
                0, "RIcloseimage", "cannot create file %s.%d.gif",
                rip->fprefix, rip->findex
            );
            return -1;
        }
        sfprintf (sfstdout, "%s.%d.gif\n", rip->fprefix, rip->findex);
        gdImageGif (rip->u.image.img, imgfp);
        fclose (imgfp);
        if (!(imfp = sfopen (NULL, sfprints (
            "%s.%d.cmap", rip->fprefix, rip->findex
        ), "w"))) {
            SUwarning (
                0, "RIcloseimage", "cannot create file %s.%d.cmap",
                rip->fprefix, rip->findex
            );
            return -1;
        }
        sfprintf (sfstdout, "%s.%d.cmap\n", rip->fprefix, rip->findex);
        for (cpass = 0, done = FALSE; !done; cpass++) {
            done = TRUE;
            for (ai = 0; ai < rip->aream; ai++) {
                ap = &rip->areas[ai];
                if (ap->pass > cpass) {
                    done = FALSE;
                    continue;
                }
                if (ap->mode & RI_AREA_AREA)
                    sfprintf (
                        imfp,
                        "<area"
                        " shape=rect coords=\"%d,%d,%d,%d\""
                        " href=\"%s\" title=\"%s\""
                        ">\n",
                        ap->u.rect.x1, ap->u.rect.y1,
                        ap->u.rect.x2, ap->u.rect.y2, ap->url,
                        ap->info ? ap->info : ap->obj
                    );
                if (ap->mode & RI_AREA_EMBED)
                    if (ap->type & RI_AREA_RECT)
                        sfprintf (
                            emfp, "%s|i|%s.%d.gif|%d|%d|%d|%d|\n",
                            ap->obj, rip->fprefix, rip->findex,
                            ap->u.rect.x1, ap->u.rect.y1,
                            ap->u.rect.x2, ap->u.rect.y2
                        );
                    else if (ap->type & RI_AREA_STYLE)
                        sfprintf (
                            emfp, "%s|s|%s|%s|%s|%s|%s|\n",
                            ap->obj,
                            ap->u.style.fn, ap->u.style.fs,
                            ap->u.style.flcl, ap->u.style.lncl, ap->u.style.fncl
                        );
            }
        }
        sfclose (imfp);
        if (!(infofp = sfopen (NULL, sfprints (
            "%s.%d.imginfo", rip->fprefix, rip->findex
        ), "w"))) {
            SUwarning (
                0, "RIcloseimage", "cannot create file %s.%d.imginfo",
                rip->fprefix, rip->findex
            );
            return -1;
        }
        sfprintf (sfstdout, "%s.%d.imginfo\n", rip->fprefix, rip->findex);
        sfprintf (infofp, "%d %d\n", rip->w, rip->h);
        sfclose (infofp);
    } else {
        for (slicei = 0; slicei < rip->u.image.slicen; slicei++) {
            h1 = (slicei * rip->h) / rip->u.image.slicen;
            h2 = ((slicei + 1) * rip->h) / rip->u.image.slicen;
            if (!(imgfp = fopen (sfprints (
                "%s.%d.%d.gif", rip->fprefix, rip->findex, slicei
            ), "w"))) {
                SUwarning (
                    0, "RIcloseimage", "cannot create file %s.%d.%d.gif",
                    rip->fprefix, rip->findex, slicei
                );
                return -1;
            }
            sfprintf (
                sfstdout, "%s.%d.%d.gif\n", rip->fprefix, rip->findex, slicei
            );
            if (!(img = gdImageCreateTrueColor (rip->w, h2 - h1))) {
                SUwarning (0, "RIcloseimage", "cannot create tmp output image");
                return -1;
            }
            gdImageCopy (
                img, rip->u.image.img, 0, 0, 0, h1, rip->w, (h2 - h1)
            );
            gdImageGif (img, imgfp);
            gdImageDestroy (img);
            fclose (imgfp);
            if (!(imfp = sfopen (NULL, sfprints (
                "%s.%d.%d.cmap", rip->fprefix, rip->findex, slicei
            ), "w"))) {
                SUwarning (
                    0, "RIcloseimage", "cannot create file %s.%d.%d.cmap",
                    rip->fprefix, rip->findex, slicei
                );
                return -1;
            }
            sfprintf (
                sfstdout, "%s.%d.%d.cmap\n", rip->fprefix, rip->findex, slicei
            );
            for (cpass = 0, done = FALSE; !done; cpass++) {
                done = TRUE;
                for (ai = 0; ai < rip->aream; ai++) {
                    ap = &rip->areas[ai];
                    if (ap->pass > cpass) {
                        done = FALSE;
                        continue;
                    }
                    y1 = ap->u.rect.y1 - h1, y2 = ap->u.rect.y2 - h1;
                    if (y2 < 0 || y1 > (h2 - h1))
                        continue;
                    if (ap->mode & RI_AREA_AREA)
                        sfprintf (
                            imfp,
                            "<area"
                            " shape=rect coords=\"%d,%d,%d,%d\""
                            " href=\"%s\" title=\"%s\""
                            ">\n",
                            ap->u.rect.x1, y1, ap->u.rect.x2, y2, ap->url,
                            ap->info ? ap->info : ap->obj
                        );
                    if (ap->mode & RI_AREA_EMBED)
                        if (ap->type & RI_AREA_RECT)
                            sfprintf (
                                emfp, "%s|i|%s.%d.gif|%d|%d|%d|%d|\n",
                                ap->obj, rip->fprefix, rip->findex,
                                ap->u.rect.x1, y1, ap->u.rect.x2, y2
                            );
                        else if (ap->type & RI_AREA_STYLE)
                            sfprintf (
                                emfp, "%s|s|%s|%s|%s|%s|%s|\n",
                                ap->obj,
                                ap->u.style.fn, ap->u.style.fs,
                                ap->u.style.flcl, ap->u.style.lncl,
                                ap->u.style.fncl
                            );
                }
            }
            sfclose (imfp);
            if (!(infofp = sfopen (NULL, sfprints (
                "%s.%d.%d.imginfo", rip->fprefix, rip->findex, slicei
            ), "w"))) {
                SUwarning (
                    0, "RIcloseimage", "cannot create file %s.%d.%d.imginfo",
                    rip->fprefix, rip->findex, slicei
                );
                return -1;
            }
            sfprintf (
                sfstdout, "%s.%d.%d.imginfo\n",
                rip->fprefix, rip->findex, slicei
            );
            sfprintf (infofp, "%d %d\n", rip->w, h2 - h1);
            sfclose (infofp);
        }
    }
    gdImageDestroy (rip->u.image.img);
    return 0;
}

static int RIclosecanvas (RI_t *rip) {
    Sfio_t *imfp, *infofp;
    RIarea_t *ap;
    int ai;
    int cpass, done, areai;

    sfprintf (rip->u.canvas.fp, "}\n");
    sfclose (rip->u.canvas.fp);

    if (!(imfp = sfopen (NULL, sfprints (
        "%s.%d.areas.js", rip->fprefix, rip->findex
    ), "w"))) {
        SUwarning (
            0, "RIclosecanvas", "cannot create file %s.%d.areas.js",
            rip->fprefix, rip->findex
        );
        return -1;
    }
    sfprintf (sfstdout, "%s.%d.areas.js\n", rip->fprefix, rip->findex);
    sfprintf (
        imfp,
        "var %s_%d_areas = null\n"
        "function %s_%d_event () {\n"
        "  if (%s_%d_areas == null) {\n"
        "    %s_%d_areas = new Array ()\n",
        rip->fprefix, rip->findex, rip->fprefix, rip->findex,
        rip->fprefix, rip->findex, rip->fprefix, rip->findex
    );
    for (areai = cpass = 0, done = FALSE; !done; cpass++) {
        done = TRUE;
        for (ai = 0; ai < rip->aream; ai++) {
            ap = &rip->areas[ai];
            if (ap->pass > cpass) {
                done = FALSE;
                continue;
            }
            if (ap->mode & RI_AREA_AREA)
                sfprintf (
                    imfp,
                    "%s_%d_areas[%d] = {\n"
                    " x1:%d, y1:%d, x2:%d, y2:%d, url:\"%s\", info:\"%s\"\n"
                    "}\n",
                    rip->fprefix, rip->findex, areai,
                    ap->u.rect.x1, ap->u.rect.y1,
                    ap->u.rect.x2, ap->u.rect.y2, ap->url,
                    ap->info ? ap->info : ap->obj
                );
            if (ap->mode & RI_AREA_EMBED)
                if (ap->type & RI_AREA_RECT)
                    sfprintf (
                        emfp, "%s|d|%s.%d.draw.js|%d|%d|%d|%d|\n",
                        ap->obj, rip->fprefix, rip->findex,
                        ap->u.rect.x1, ap->u.rect.y1,
                        ap->u.rect.x2, ap->u.rect.y2
                    );
                else if (ap->type & RI_AREA_STYLE)
                    sfprintf (
                        emfp, "%s|s|%s|%s|%s|%s|%s|\n",
                        ap->obj,
                        ap->u.style.fn, ap->u.style.fs,
                        ap->u.style.flcl, ap->u.style.lncl, ap->u.style.fncl
                    );
        }
    }
    sfclose (imfp);
    if (!(infofp = sfopen (NULL, sfprints (
        "%s.%d.imginfo", rip->fprefix, rip->findex
    ), "w"))) {
        SUwarning (
            0, "RIclosecanvas", "cannot create file %s.%d.imginfo",
            rip->fprefix, rip->findex
        );
        return -1;
    }
    sfprintf (sfstdout, "%s.%d.imginfo\n", rip->fprefix, rip->findex);
    sfprintf (infofp, "%d %d\n", rip->w, rip->h);
    sfclose (infofp);
    return 0;
}

static int RIclosetable (RI_t *rip) {
    Sfio_t *cssfp;
    RIcss_t *cp;

    if (!(cssfp = sfopen (NULL, sfprints (
        "%s.%d.css", rip->fprefix, rip->findex
    ), "w"))) {
        SUwarning (
            0, "RIclosetable", "cannot create file %s.%d.css",
            rip->fprefix, rip->findex
        );
        return -1;
    }
    sfprintf (cssfp, "<style type='text/css'>\n");
    for (
        cp = dtfirst (rip->u.table.cssdict); cp;
        cp = dtnext (rip->u.table.cssdict, cp)
    )
        sfprintf (cssfp, "%s\n", cp->str);
    sfprintf (cssfp, "</style>\n");
    sfclose (cssfp);
    sfclose (rip->u.table.fp);
    return 0;
}

int RIaddarea (RI_t *rip, RIarea_t *iap) {
    RIarea_t *ap;

    if (rip->aream >= rip->arean) {
        if (!(rip->areas = vmresize (
            rip->vm, rip->areas,
            (rip->arean + 100) * sizeof (RIarea_t), VM_RSCOPY
        ))) {
            SUwarning (0, "RIaddarea", "cannot grow areas array");
            return -1;
        }
        rip->arean += 100;
    }
    ap = &rip->areas[rip->aream++];
    *ap = *iap;
    ap->obj = NULL;
    ap->url = NULL;
    ap->info = NULL;
    if (iap->obj && !(ap->obj = vmstrdup (rip->vm, iap->obj))) {
        SUwarning (0, "RIaddarea", "cannot copy obj");
        return -1;
    }
    if (iap->url && !(ap->url = vmstrdup (rip->vm, iap->url))) {
        SUwarning (0, "RIaddarea", "cannot copy url");
        return -1;
    }
    if (iap->info && !(ap->info = vmstrdup (rip->vm, iap->info))) {
        SUwarning (0, "RIaddarea", "cannot copy info");
        return -1;
    }
    return 0;
}

char *RIaddcss (RI_t *rip, char *tag, char *fn, char *fs, char *bg, char *cl) {
    RIcss_t *cmem, *cp;
    char buf[1024], *s1, *s2, c;

    strcpy (buf, tag);
    sfstruse (strfp);
    sfprintf (strfp, "%s;%s;%s;%s;%s;%s", tag, fileprefix, fn, fs, bg, cl);
    if (!(cp = dtmatch (rip->u.table.cssdict, (s1 = sfstruse (strfp))))) {
        if (!(cmem = vmalloc (rip->vm, sizeof (RIcss_t)))) {
            SUwarning (0, "RIaddcss", "cannot allocate cmem");
            return NULL;
        }
        memset (cmem, 0, sizeof (RIcss_t));
        if (!(cmem->key = vmstrdup (rip->vm, s1))) {
            SUwarning (0, "RIaddcss", "cannot copy key");
            return NULL;
        }
        if ((cp = dtinsert (rip->u.table.cssdict, cmem)) != cmem) {
            SUwarning (0, "RIaddcss", "cannot insert css");
            return NULL;
        }
        for (s1 = buf; s1; s1 = s2) {
            s2 = strchr (s1, '|');
            if (s2)
                c = *s2, *s2 = 0;
            sfprintf (
                strfp,
                "%s.%s_%d {"
                " font-family:%s; font-size:%s;"
                " background-color:%s; color:%s;"
                " border-spacing:2px; border-collapse:separate "
                "}\n",
                s1, fileprefix, cssid, fn, fs, bg, cl
            );
            if (s2)
                *s2 = c, s2 += 1;
        }
        if (!(cp->str = vmstrdup (rip->vm, sfstruse (strfp)))) {
            SUwarning (0, "RIaddcss", "cannot copy str");
            return NULL;
        }
        sfprintf (strfp, "%s_%d", fileprefix, cssid);
        if (!(cp->tid = vmstrdup (rip->vm, sfstruse (strfp)))) {
            SUwarning (0, "RIaddcss", "cannot copy tid");
            return NULL;
        }
        cssid++;
    }
    return cp->tid;
}

static int RIaddimagetoc (RI_t *, int, char *);
static int RIaddcanvastoc (RI_t *, int, char *);
static int RIaddtabletoc (RI_t *, int, char *);

int RIaddtoc (RI_t *rip, int index, char *text) {
    switch (rip->type) {
    case RI_TYPE_IMAGE:
        return RIaddimagetoc (rip, index, text);
    case RI_TYPE_CANVAS:
        return RIaddcanvastoc (rip, index, text);
    case RI_TYPE_TABLE:
        return RIaddtabletoc (rip, index, text);
    default:
        SUwarning (0, "RIaddtoc", "unknown RI type %d", rip->type);
        return -1;
    }
    return 0;
}

static int RIaddimagetoc (RI_t *rip, int index, char *text) {
    sfprintf (
        rip->tocfp, "%d|%s.%d.gif|%s\n",
        index, rip->fprefix, rip->findex, text
    );
    return 0;
}

static int RIaddcanvastoc (RI_t *rip, int index, char *text) {
    sfprintf (
        rip->tocfp, "%d|%s.%d.gif|%s\n",
        index, rip->fprefix, rip->findex, text
    );
    return 0;
}

static int RIaddtabletoc (RI_t *rip, int index, char *text) {
    RIop_t op;
    char buf[1000];

    if (text)
        sfprintf (
            rip->tocfp, "%d|%s.%d.html|%s\n",
            index, rip->fprefix, rip->findex, text
        );
    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLCOM;
    sfsprintf (buf, 1000, "page %d", index);
    op.u.hcom.str = buf;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "RIaddtabletoc", "cannot add comment");
        return -1;
    }
    return 0;
}

int RIparseattr (char *str, int type, RIop_t *opp) {
    char *x1p, *y1p, *x2p, *y2p, *whp;
    int len;

    if ((len = strlen (str) + 1) >= attrlen) {
        if (!(attrstr = vmresize (Vmheap, attrstr, len, VM_RSCOPY))) {
            SUwarning (0, "RIparseattr", "cannot grow attrstr array");
            return -1;
        }
        attrlen = len;
    }
    memcpy (attrstr, str, len);
    switch (type) {
    case RI_OP_BB:
        x1p = attrstr;
        y1p = strchr (x1p, ',');
        x2p = (y1p) ? strchr (y1p + 1, ',') : NULL;
        y2p = (x2p) ? strchr (x2p + 1, ',') : NULL;
        if (!y2p) {
            SUwarning (0, "RIparseattr", "incomplete dir %s", str);
            return -1;
        }
        *y1p++ = 0;
        *x2p++ = 0;
        *y2p++ = 0;
        opp->type = RI_OP_BB;
        opp->u.rect.o.x = S (atoi (x1p)), opp->u.rect.o.y = S (atoi (y1p));
        opp->u.rect.s.x = S (atoi (x2p)) - opp->u.rect.o.x;
        opp->u.rect.s.y = S (atoi (y2p)) - opp->u.rect.o.y;
        break;
    case RI_OP_POS:
        x1p = attrstr;
        y1p = strchr (x1p, ',');
        if (!y1p) {
            SUwarning (0, "RIparseattr", "incomplete dir %s", str);
            return -1;
        }
        *y1p++ = 0;
        opp->type = RI_OP_POS;
        opp->u.point.x = S (atoi (x1p)), opp->u.point.y = S (atoi (y1p));
        break;
    case RI_OP_SIZ:
        x1p = attrstr;
        y1p = strchr (x1p, ',');
        if (!y1p) {
            SUwarning (0, "RIparseattr", "incomplete dir %s", str);
            return -1;
        }
        *y1p++ = 0;
        opp->type = RI_OP_SIZ;
        opp->u.point.x = S (atoi (x1p)), opp->u.point.y = S (atoi (y1p));
        break;
    case RI_OP_LEN:
        whp = attrstr;
        opp->type = RI_OP_LEN;
        opp->u.length.l = S (72.0 * atof (whp));
        break;
    }
    return 0;
}

int RIparseop (RI_t *rip, char *str, int rflag) {
    RIop_t *opp;
    RIpoint_t *pp;
    char *tstr;
    char *cxp, *cyp, *sxp, *syp, *pnp, *pxp, *pyp, *txp, *typ, *tjp, *twp, *tnp;
    char *cnp, *fsp, *fnp, *snp;
    int pn, pi;
    int len, l;

    if ((len = strlen (str) + 1) >= attrlen) {
        if (!(attrstr = vmresize (Vmheap, attrstr, len, VM_RSCOPY))) {
            SUwarning (0, "RIparseop", "cannot grow attrstr array");
            return -1;
        }
        attrlen = len;
    }
    memcpy (attrstr, str, len);
    if (rflag) {
        rip->opm = 0, rip->cflag = FALSE, rip->pointm = 0;
    }
    TOKINIT (attrstr);
    for (;;) {
        NTOK;
        if (!tokp || *tokp == 0)
            break;

        if (rip->opm + 1 >= rip->opn) {
            if (!(rip->ops = vmresize (
                rip->vm, rip->ops, (rip->opn + 100) * sizeof (RIop_t), VM_RSCOPY
            ))) {
                SUwarning (0, "RIparseop", "cannot grow ops array");
                return -1;
            }
            rip->opn += 100;
        }
        opp = &rip->ops[rip->opm++];
        memset (opp, 0, sizeof (RIop_t));
        tstr = tokp;
        switch (*tstr) {
        case 'E':
        case 'e':
            opp->type = (*tstr == 'E') ? RI_OP_E : RI_OP_e;
            NTOK; cxp = tokp; NTOK; cyp = tokp;
            NTOK; sxp = tokp; NTOK; syp = tokp;
            if (!syp) {
                SUwarning (0, "RIparseop", "incomplete dir %s", tstr);
                return -1;
            }
            opp->u.rect.o.x = X (atoi (cxp)), opp->u.rect.o.y = Y (atoi (cyp));
            opp->u.rect.o.x += rip->u.image.vpx;
            opp->u.rect.o.y += rip->u.image.vpy;
            opp->u.rect.s.x = S (atoi (sxp)), opp->u.rect.s.y = S (atoi (syp));
            opp->u.rect.ba = 0, opp->u.rect.ea = 360;
            if (*tstr == 'E') {
                opp = &rip->ops[rip->opm++];
                *opp = rip->ops[rip->opm - 2];
                opp->type = RI_OP_e;
            }
            break;
        case 'P':
        case 'p':
        case 'L':
            opp->type = (*tstr == 'P') ? RI_OP_P : (
                (*tstr == 'p') ? RI_OP_p : RI_OP_L
            );
            NTOK; pnp = tokp;
            if (!pnp) {
                SUwarning (0, "RIparseop", "incomplete dir %s", tstr);
                return -1;
            }
            pn = atoi (pnp);
            if (rip->pointm + pn >= rip->pointn) {
                if (!(rip->points = vmresize (
                    rip->vm, rip->points,
                    (rip->pointn + pn) * sizeof (RIpoint_t), VM_RSCOPY
                ))) {
                    SUwarning (0, "RIparseop", "cannot grow point array");
                    return -1;
                }
                rip->pointn += pn;
            }
            opp->u.poly.pn = pn, opp->u.poly.pi = rip->pointm;
            for (pi = 0; pi < pn; pi++) {
                NTOK; pxp = tokp; NTOK; pyp = tokp;
                if (!pyp) {
                    SUwarning (0, "RIparseop", "incomplete dir %s", tstr);
                    return -1;
                }
                pp = &rip->points[rip->pointm++];
                pp->x = X (atoi (pxp)), pp->y = Y (atoi (pyp));
                pp->x += rip->u.image.vpx;
                pp->y += rip->u.image.vpy;
            }
            if (*tstr == 'P') {
                opp = &rip->ops[rip->opm++];
                *opp = rip->ops[rip->opm - 2];
                opp->type = RI_OP_p;
            }
            break;
        case 'B':
        case 'b':
            opp->type = (*tstr == 'B') ? RI_OP_B : RI_OP_b;
            NTOK; pnp = tokp;
            if (!pnp) {
                SUwarning (0, "RIparseop", "incomplete dir %s", tstr);
                return -1;
            }
            pn = atoi (pnp);
            if (rip->pointm + pn >= rip->pointn) {
                if (!(rip->points = vmresize (
                    rip->vm, rip->points,
                    (rip->pointn + pn) * sizeof (RIpoint_t), VM_RSCOPY
                ))) {
                    SUwarning (0, "RIparseop", "cannot grow point array");
                    return -1;
                }
                rip->pointn += pn;
            }
            opp->u.poly.pn = pn, opp->u.poly.pi = rip->pointm;
            for (pi = 0; pi < pn; pi++) {
                NTOK; pxp = tokp; NTOK; pyp = tokp;
                if (!pyp) {
                    SUwarning (0, "RIparseop", "incomplete dir %s", tstr);
                    return -1;
                }
                pp = &rip->points[rip->pointm++];
                pp->x = X (atoi (pxp)), pp->y = Y (atoi (pyp));
                pp->x += rip->u.image.vpx;
                pp->y += rip->u.image.vpy;
            }
            if (*tstr == 'b') {
                opp = &rip->ops[rip->opm++];
                *opp = rip->ops[rip->opm - 2];
                opp->type = RI_OP_b;
            }
            break;
        case 'T':
            opp->type = RI_OP_T;
            NTOK; txp = tokp; NTOK; typ = tokp;
            NTOK; tjp = tokp;
            NTOK; twp = tokp;
            NTOK; tnp = tokp;
            if (!tnp) {
                SUwarning (0, "RIparseop", "incomplete dir %s", typ);
                return -1;
            }
            opp->u.text.p.x = X (atoi (txp)), opp->u.text.p.y = Y (atoi (typ));
            opp->u.text.p.x += rip->u.image.vpx;
            opp->u.text.p.y += rip->u.image.vpy;
            opp->u.text.jx = atoi (tjp);
            opp->u.text.jy = 2;
            opp->u.text.w = S (atoi (twp));
            opp->u.text.n = atoi (tnp);
            NTOKL (opp->u.text.n + 1);
            if (!(opp->u.text.t = vmstrdup (rip->vm, tokp + 1))) {
                SUwarning (0, "RIparseop", "cannot copy text %s", tokp + 1);
                return -1;
            }
            break;
        case 'F':
            opp->type = RI_OP_F;
            NTOK; fsp = tokp;
            NTOK; fnp = tokp;
            if (!fnp) {
                SUwarning (0, "RIparseop", "incomplete dir %s", typ);
                return -1;
            }
            opp->u.font.s = atoi (fsp);
            opp->u.font.n = atoi (fnp);
            NTOKL (opp->u.font.n + 1);
            if (!(opp->u.font.t = vmstrdup (rip->vm, tokp + 1))) {
                SUwarning (0, "RIparseop", "cannot copy font %s", tokp + 1);
                return -1;
            }
            opp->u.font.f.type = RI_FONT_NONE;
            break;
        case 'C':
            opp->type = RI_OP_C;
            NTOK; cnp = tokp;
            if (!cnp) {
                SUwarning (0, "RIparseop", "incomplete dir %s", typ);
                return -1;
            }
            opp->u.color.n = atoi (cnp);
            NTOKL (opp->u.color.n + 1);
            if (!(opp->u.color.t = vmstrdup (rip->vm, tokp + 1))) {
                SUwarning (0, "RIparseop", "cannot copy C color %s", tokp + 1);
                return -1;
            }
            if (opp->u.color.t[0] == '#' && strlen (opp->u.color.t) == 9)
                opp->u.color.t[7] = 0;
            break;
        case 'c':
            opp->type = RI_OP_c;
            NTOK; cnp = tokp;
            if (!cnp) {
                SUwarning (0, "RIparseop", "incomplete dir %s", typ);
                return -1;
            }
            opp->u.color.n = atoi (cnp);
            NTOKL (opp->u.color.n + 1);
            if (!(opp->u.color.t = vmstrdup (rip->vm, tokp + 1))) {
                SUwarning (0, "RIparseop", "cannot copy c color %s", tokp + 1);
                return -1;
            }
            if (opp->u.color.t[0] == '#' && strlen (opp->u.color.t) == 9)
                opp->u.color.t[7] = 0;
            opp->u.color.locked = TRUE;
            break;
        case 'S':
            opp->type = RI_OP_S;
            NTOK; snp = tokp;
            if (!snp) {
                SUwarning (0, "RIparseop", "incomplete dir %s", typ);
                return -1;
            }
            opp->u.style.n = atoi (snp);
            NTOKL (opp->u.style.n + 1);
            if (!(opp->u.style.t = vmstrdup (rip->vm, tokp + 1))) {
                SUwarning (0, "RIparseop", "cannot copy style %s", tokp + 1);
                return -1;
            }
            opp->u.style.style = RI_STYLE_NONE;
            if (strcmp (opp->u.style.t, "dotted") == 0)
                opp->u.style.style = RI_STYLE_DOTTED;
            else if (strcmp (opp->u.style.t, "dashed") == 0)
                opp->u.style.style = RI_STYLE_DASHED;
            else if (strncmp (opp->u.style.t, "setlinewidth(", 13) == 0) {
                opp->u.style.style = RI_STYLE_LINEWIDTH;
                l = strlen (opp->u.style.t);
                memccpy (opp->u.style.t, opp->u.style.t + 13, 0, l - 13);
            }
            break;
        case 'I':
            opp->type = RI_OP_NOOP;
            break;
        }
    }
    return 0;
}

int RIaddop (RI_t *rip, RIop_t *addopp) {
    RIop_t *opp;

    if (!addopp) {
        rip->opm = 0, rip->cflag = FALSE, rip->pointm = 0;
        return 0;
    }

    if (rip->opm >= rip->opn) {
        if (!(rip->ops = vmresize (
            rip->vm, rip->ops, (rip->opn + 100) * sizeof (RIop_t), VM_RSCOPY
        ))) {
            SUwarning (0, "RIaddopp", "cannot grow ops array");
            return -1;
        }
        rip->opn += 100;
    }
    opp = &rip->ops[rip->opm++];
    memset (opp, 0, sizeof (RIop_t));

    opp->type = addopp->type;
    switch (addopp->type) {
    case RI_OP_E:
    case RI_OP_e:
        opp->u.rect.o.x = rip->u.image.vpx + addopp->u.rect.o.x;
        opp->u.rect.o.y = rip->u.image.vpy + addopp->u.rect.o.y;
        opp->u.rect.s.x = addopp->u.rect.s.x;
        opp->u.rect.s.y = addopp->u.rect.s.x;
        opp->u.rect.ba = addopp->u.rect.ba;
        opp->u.rect.ea = addopp->u.rect.ea;
        break;
    case RI_OP_P:
    case RI_OP_p:
    case RI_OP_L:
    case RI_OP_B:
    case RI_OP_b:
        opp->u.poly.pn = addopp->u.poly.pn, opp->u.poly.pi = addopp->u.poly.pi;
        break;
    case RI_OP_T:
        opp->u.text.p.x = rip->u.image.vpx + addopp->u.text.p.x;
        opp->u.text.p.y = rip->u.image.vpy + addopp->u.text.p.y;
        opp->u.text.jx = addopp->u.text.jx;
        opp->u.text.jy = addopp->u.text.jy;
        opp->u.text.w = addopp->u.text.w;
        opp->u.text.h = addopp->u.text.h;
        opp->u.text.n = addopp->u.text.n;
        if (!(opp->u.text.t = vmstrdup (rip->vm, addopp->u.text.t))) {
            SUwarning (0, "RIaddop", "cannot copy text");
            return -1;
        }
        break;
    case RI_OP_F:
        opp->u.font.s = addopp->u.font.s;
        opp->u.font.n = addopp->u.font.n;
        if (addopp->u.font.t && !(
            opp->u.font.t = vmstrdup (rip->vm, addopp->u.font.t)
        )) {
            SUwarning (0, "RIaddop", "cannot copy text");
            return -1;
        }
        opp->u.font.f = addopp->u.font.f;
        break;
    case RI_OP_C:
    case RI_OP_COPYC:
        if (addopp->type == RI_OP_COPYC)
            rip->cflag = TRUE;
        opp->u.color.n = addopp->u.color.n;
        if (!(opp->u.color.t = vmstrdup (rip->vm, addopp->u.color.t))) {
            SUwarning (0, "RIaddop", "cannot copy text");
            return -1;
        }
        opp->u.color.locked = addopp->u.color.locked;
        break;
    case RI_OP_c:
    case RI_OP_COPYc:
        if (addopp->type == RI_OP_COPYc)
            rip->cflag = TRUE;
        opp->u.color.n = addopp->u.color.n;
        if (!(opp->u.color.t = vmstrdup (rip->vm, addopp->u.color.t))) {
            SUwarning (0, "RIaddop", "cannot copy text");
            return -1;
        }
        opp->u.color.locked = addopp->u.color.locked;
        break;
    case RI_OP_FC:
    case RI_OP_COPYFC:
        if (addopp->type == RI_OP_COPYFC)
            rip->cflag = TRUE;
        opp->u.color.n = addopp->u.color.n;
        if (!(opp->u.color.t = vmstrdup (rip->vm, addopp->u.color.t))) {
            SUwarning (0, "RIaddop", "cannot copy text");
            return -1;
        }
        opp->u.color.locked = addopp->u.color.locked;
        break;
    case RI_OP_S:
        opp->u.style.n = addopp->u.style.n;
        if (!(opp->u.style.t = vmstrdup (rip->vm, addopp->u.style.t))) {
            SUwarning (0, "RIaddop", "cannot copy text");
            return -1;
        }
        opp->u.style.style = addopp->u.style.style;
        break;
    case RI_OP_I:
        opp->u.img.img = addopp->u.img.img;
        opp->u.img.rx = rip->u.image.vpx + addopp->u.img.rx;
        opp->u.img.ry = rip->u.image.vpy + addopp->u.img.ry;
        opp->u.img.ix = addopp->u.img.ix, opp->u.img.iy = addopp->u.img.iy;
        opp->u.img.iw = addopp->u.img.iw, opp->u.img.ih = addopp->u.img.ih;
        break;
    case RI_OP_POS:
        opp->u.point.x = addopp->u.point.x, opp->u.point.y = addopp->u.point.y;
        break;
    case RI_OP_SIZ:
        opp->u.point.x = addopp->u.point.x, opp->u.point.y = addopp->u.point.y;
        break;
    case RI_OP_LW:
        opp->u.length.l = addopp->u.length.l;
        break;
    case RI_OP_CLIP:
        opp->u.rect.o.x = addopp->u.rect.o.x;
        opp->u.rect.o.y = addopp->u.rect.o.y;
        opp->u.rect.s.x = addopp->u.rect.s.x;
        opp->u.rect.s.y = addopp->u.rect.s.y;
        break;
    case RI_OP_HTMLTBB:
    case RI_OP_HTMLTBE:
    case RI_OP_HTMLTBCB:
    case RI_OP_HTMLTBCE:
        if (addopp->u.htb.lab && !(opp->u.htb.lab = vmstrdup (
            rip->vm, htmlreenc (addopp->u.htb.lab)
        ))) {
            SUwarning (0, "RIaddop", "cannot copy lab");
            return -1;
        }
        if (addopp->u.htb.fn && !(opp->u.htb.fn = vmstrdup (
            rip->vm, addopp->u.htb.fn
        ))) {
            SUwarning (0, "RIaddop", "cannot copy fn");
            return -1;
        }
        if (addopp->u.htb.fs && !(opp->u.htb.fs = vmstrdup (
            rip->vm, addopp->u.htb.fs
        ))) {
            SUwarning (0, "RIaddop", "cannot copy fs");
            return -1;
        }
        if (addopp->u.htb.bg && !(opp->u.htb.bg = vmstrdup (
            rip->vm, addopp->u.htb.bg
        ))) {
            SUwarning (0, "RIaddop", "cannot copy bg");
            return -1;
        }
        if (addopp->u.htb.cl && !(opp->u.htb.cl = vmstrdup (
            rip->vm, addopp->u.htb.cl
        ))) {
            SUwarning (0, "RIaddop", "cannot copy cl");
            return -1;
        }
        if (addopp->u.htb.url && !(opp->u.htb.url = vmstrdup (
            rip->vm, addopp->u.htb.url
        ))) {
            SUwarning (0, "RIaddop", "cannot copy url");
            return -1;
        }
        if (opp->type == RI_OP_HTMLTBB || opp->type == RI_OP_HTMLTBCB) {
            if (!(opp->u.htb.tid = RIaddcss (
                rip, (opp->type == RI_OP_HTMLTBB) ? "table" : "caption",
                opp->u.htb.fn, opp->u.htb.fs, opp->u.htb.bg, opp->u.htb.cl
            ))) {
                SUwarning (0, "RIaddop", "cannot add css");
                return -1;
            }
        }
        break;
    case RI_OP_HTMLTRB:
    case RI_OP_HTMLTRE:
        if (addopp->u.htr.fn && !(opp->u.htr.fn = vmstrdup (
            rip->vm, addopp->u.htr.fn
        ))) {
            SUwarning (0, "RIaddop", "cannot copy fn");
            return -1;
        }
        if (addopp->u.htr.fs && !(opp->u.htr.fs = vmstrdup (
            rip->vm, addopp->u.htr.fs
        ))) {
            SUwarning (0, "RIaddop", "cannot copy fs");
            return -1;
        }
        if (addopp->u.htr.bg && !(opp->u.htr.bg = vmstrdup (
            rip->vm, addopp->u.htr.bg
        ))) {
            SUwarning (0, "RIaddop", "cannot copy bg");
            return -1;
        }
        if (addopp->u.htr.cl && !(opp->u.htr.cl = vmstrdup (
            rip->vm, addopp->u.htr.cl
        ))) {
            SUwarning (0, "RIaddop", "cannot copy cl");
            return -1;
        }
        if (opp->type == RI_OP_HTMLTRB && !(opp->u.htr.tid = RIaddcss (
            rip, "tr|a", opp->u.htr.fn, opp->u.htr.fs,
            opp->u.htr.bg, opp->u.htr.cl
        ))) {
            SUwarning (0, "RIaddop", "cannot add css");
            return -1;
        }
        break;
    case RI_OP_HTMLTDB:
    case RI_OP_HTMLTDE:
    case RI_OP_HTMLTDT:
        if (addopp->u.htd.lab && !(opp->u.htd.lab = vmstrdup (
            rip->vm, htmlreenc (addopp->u.htd.lab)
        ))) {
            SUwarning (0, "RIaddop", "cannot copy lab");
            return -1;
        }
        if (addopp->u.htd.fn && !(opp->u.htd.fn = vmstrdup (
            rip->vm, addopp->u.htd.fn
        ))) {
            SUwarning (0, "RIaddop", "cannot copy fn");
            return -1;
        }
        if (addopp->u.htd.fs && !(opp->u.htd.fs = vmstrdup (
            rip->vm, addopp->u.htd.fs
        ))) {
            SUwarning (0, "RIaddop", "cannot copy fs");
            return -1;
        }
        if (addopp->u.htd.bg && !(opp->u.htd.bg = vmstrdup (
            rip->vm, addopp->u.htd.bg
        ))) {
            SUwarning (0, "RIaddop", "cannot copy bg");
            return -1;
        }
        if (addopp->u.htd.cl && !(opp->u.htd.cl = vmstrdup (
            rip->vm, addopp->u.htd.cl
        ))) {
            SUwarning (0, "RIaddop", "cannot copy cl");
            return -1;
        }
        if (opp->type == RI_OP_HTMLTDB && !(opp->u.htd.tid = RIaddcss (
            rip, "td", opp->u.htd.fn, opp->u.htd.fs,
            opp->u.htd.bg, opp->u.htd.cl
        ))) {
            SUwarning (0, "RIaddop", "cannot add css");
            return -1;
        }
        opp->u.htd.j = addopp->u.htd.j;
        opp->u.htd.cn = addopp->u.htd.cn;
        break;
    case RI_OP_HTMLAB:
    case RI_OP_HTMLAE:
        if (addopp->u.ha.url && !(opp->u.ha.url = vmstrdup (
            rip->vm, addopp->u.ha.url
        ))) {
            SUwarning (0, "RIaddop", "cannot copy url");
            return -1;
        }
        if (addopp->u.ha.info && !(opp->u.ha.info = vmstrdup (
            rip->vm, addopp->u.ha.info
        ))) {
            SUwarning (0, "RIaddop", "cannot copy info");
            return -1;
        }
        if (addopp->u.ha.fn && !(opp->u.ha.fn = vmstrdup (
            rip->vm, addopp->u.ha.fn
        ))) {
            SUwarning (0, "RIaddop", "cannot copy fn");
            return -1;
        }
        if (addopp->u.ha.fs && !(opp->u.ha.fs = vmstrdup (
            rip->vm, addopp->u.ha.fs
        ))) {
            SUwarning (0, "RIaddop", "cannot copy fs");
            return -1;
        }
        if (addopp->u.ha.bg && !(opp->u.ha.bg = vmstrdup (
            rip->vm, addopp->u.ha.bg
        ))) {
            SUwarning (0, "RIaddop", "cannot copy bg");
            return -1;
        }
        if (addopp->u.ha.cl && !(opp->u.ha.cl = vmstrdup (
            rip->vm, addopp->u.ha.cl
        ))) {
            SUwarning (0, "RIaddop", "cannot copy cl");
            return -1;
        }
        if (opp->type == RI_OP_HTMLAB && !(opp->u.ha.tid = RIaddcss (
            rip, "a", opp->u.ha.fn, opp->u.ha.fs, opp->u.ha.bg, opp->u.ha.cl
        ))) {
            SUwarning (0, "RIaddop", "cannot add css");
            return -1;
        }
        break;
    case RI_OP_HTMLCOM:
        if (addopp->u.hcom.str && !(opp->u.hcom.str = vmstrdup (
            rip->vm, htmlreenc (addopp->u.hcom.str)
        ))) {
            SUwarning (0, "RIaddop", "cannot copy str");
            return -1;
        }
        break;
    }

    return 0;
}

int RIcenterops (RIop_t *ops, int opn, int nw, int nh, int aw, int ah) {
    int opi;
    int dw, dh;

    dw = (aw - nw) / 2, dh = (ah - nh) / 2;
    if (dw < 0)
        dw = 0;
    if (dh < 0)
        dh = 0;
    if (dw == 0 || dh == 0)
        return 0;

    for (opi = 0; opi < opn; opi++) {
        if (ops[opi].type == RI_OP_I) {
            ops[opi].u.img.rx += dw;
            ops[opi].u.img.ry += dh;
        } else if (ops[opi].type == RI_OP_T) {
            ops[opi].u.text.p.x += dw;
            ops[opi].u.text.p.y += dh;
        }
    }
    return 0;
}

int RIaddpt (RI_t *rip, int x, int y) {
    RIpoint_t *pp;

    if (rip->pointm >= rip->pointn) {
        if (!(rip->points = vmresize (
            rip->vm, rip->points,
            (rip->pointn + 100) * sizeof (RIpoint_t), VM_RSCOPY
        ))) {
            SUwarning (0, "RIaddpt", "cannot grow point array");
            return -1;
        }
        rip->pointn += 100;
    }
    pp = &rip->points[rip->pointm];
    pp->x = rip->u.image.vpx + x; pp->y = rip->u.image.vpy + y;
    return rip->pointm++;
}

int RIaddpts (RI_t *rip, int *xs, int *ys, int pn) {
    RIpoint_t *pp;
    int pi, pm;

    pm = rip->pointm;
    if (rip->pointm + pn >= rip->pointn) {
        if (!(rip->points = vmresize (
            rip->vm, rip->points,
            (rip->pointn + pn) * sizeof (RIpoint_t), VM_RSCOPY
        ))) {
            SUwarning (0, "RIaddpts", "cannot grow point array");
            return -1;
        }
        rip->pointn += pn;
    }
    for (pi = 0; pi < pn; pi++) {
        pp = &rip->points[rip->pointm++];
        pp->x = rip->u.image.vpx + xs[pi]; pp->y = rip->u.image.vpy + ys[pi];
    }
    return pm;
}

int RIgettextsize (char *str, char *fname, char *fsize, RIop_t *opp) {
    RIfont_t f;
    int r[8], size, w;
    char *s1, *s2, *s3, c;

    size = atoi (fsize);
    findfont (fname, size, &f);
    memset (opp, 0, sizeof (RIop_t));
    opp->type = RI_OP_FI;
    opp->u.font.f = f;
    opp->u.font.w = 0;
    opp->u.font.h = 0;
    opp->u.font.t = fname;
    opp->u.font.s = size;
    if (f.type == RI_FONT_TTF) {
        for (s1 = str; s1; s1 = s2) {
            s2 = strstr (s1, "\\n");
            if (s2)
                c = *s2, *s2 = 0;
            if ((s3 = gdImageStringFT (
                NULL, &r[0], 0, f.file, size, 0.0, 0.0, 0.0, s1
            ))) {
                SUwarning (0, "RIgettextsize", "font error: %s", s3);
                return -1;
            }
            if (opp->u.font.w < r[4] - r[0])
                opp->u.font.w = r[4] - r[0];
            if (RImaxtsflag && (s3 = gdImageStringFT (
                NULL, &r[0], 0, f.file, size, 0.0, 0.0, 0.0, "Wy"
            ))) {
                SUwarning (0, "RIgettextsize", "font error: %s", s3);
                return -1;
            }
            opp->u.font.h += (r[1] - r[5]) + 2;
            if (s2)
                *s2 = c, s2 += 2;
        }
    } else {
        for (s1 = str; s1; s1 = s2) {
            s2 = strstr (s1, "\\n");
            if (s2)
                c = *s2, *s2 = 0;
            w = f.font->w * strlen (s1);
            if (opp->u.font.w < w)
                opp->u.font.w = w;
            opp->u.font.h += f.font->h + 2;
            if (s2)
                *s2 = c, s2 += 2;
        }
    }
    return 0;
}

int RIsetviewport (RI_t *rip, int flag, int x, int y, int w, int h) {
    if (!flag) {
        rip->u.image.vpx = 0, rip->u.image.vpy = 0;
        rip->u.image.vpw = rip->w, rip->u.image.vph = rip->h;
    } else {
        rip->u.image.vpx = x, rip->u.image.vpy = y;
        rip->u.image.vpw = w, rip->u.image.vph = h;
    }
    return 0;
}

static int RIexecimageop (
    RI_t *, RIop_t *, int, char *, char *, char *, char *, char *
);
static int RIexeccanvasop (
    RI_t *, RIop_t *, int, char *, char *, char *, char *, char *
);
static int RIexectableop (
    RI_t *, RIop_t *, int, char *, char *, char *, char *
);

int RIexecop (
    RI_t *rip, RIop_t *iops, int iopm,
    char *bgcolor, char *color, char *flcolor, char *fncolor,
    char *fnname, char *fnsize
) {
    RIop_t *ops;
    int opm;

    if (iops)
        ops = iops, opm = iopm;
    else
        ops = rip->ops, opm = rip->opm;

    switch (rip->type) {
    case RI_TYPE_IMAGE:
        return RIexecimageop (
            rip, ops, opm, color, flcolor, fncolor, fnname, fnsize
        );
    case RI_TYPE_CANVAS:
        return RIexeccanvasop (
            rip, ops, opm, color, flcolor, fncolor, fnname, fnsize
        );
    case RI_TYPE_TABLE:
        return RIexectableop (
            rip, ops, opm, bgcolor, color, fnname, fnsize
        );
    default:
        SUwarning (0, "RIexecop", "unknown RI type %d", rip->type);
        return -1;
    }
    return 0;
}

static int RIexecimageop (
    RI_t *rip, RIop_t *ops, int opm,
    char *lncolor, char *flcolor, char *fncolor,
    char *fnname, char *fnsize
) {
    RIop_t *opp;
    int opi, opj;
    int lnid, flid, fnid, fnsz, x, y, dx, dy, r[8], l1;
    RIfont_t font;
    char *s1, *s2, *s3, c;
    int stn, sti, sts[10];

    if (rip->cflag) {
        for (opi = 0; opi < opm; opi++) {
            opp = &ops[opi];
            if (
                opp->type != RI_OP_COPYC && opp->type != RI_OP_COPYc &&
                opp->type != RI_OP_COPYFC
            )
                continue;
            if (opp->type == RI_OP_COPYc)
                lncolor = opp->u.color.t;
            else if (opp->type == RI_OP_COPYC)
                flcolor = opp->u.color.t;
            else if (opp->type == RI_OP_COPYFC)
                fncolor = opp->u.color.t;
            for (opj = 0; opj < opm; opj++) {
                if ((
                    opp->type == RI_OP_COPYC && ops[opj].type == RI_OP_C &&
                    !ops[opj].u.color.locked
                ) || (
                    opp->type == RI_OP_COPYc && ops[opj].type == RI_OP_c &&
                    !ops[opj].u.color.locked
                ) || (
                    opp->type == RI_OP_COPYFC && ops[opj].type == RI_OP_FC &&
                    !ops[opj].u.color.locked
                ))
                    ops[opj].u.color.t = opp->u.color.t;
            }
        }
    }

    lnid = parsecolor (rip, lncolor);
    flid = parsecolor (rip, flcolor);
    fnid = parsecolor (rip, fncolor);
    fnsz = atoi (fnsize);
    findfont (fnname, fnsz, &font);
    stn = 0;
    for (opi = 0; opi < opm; opi++) {
        opp = &ops[opi];
        switch (opp->type) {
        case RI_OP_E:
            gdImageFilledArc (
                rip->u.image.img, opp->u.rect.o.x, opp->u.rect.o.y,
                2 * opp->u.rect.s.x, 2 * opp->u.rect.s.y,
                opp->u.rect.ba, opp->u.rect.ea, flid, 0
            );
            break;
        case RI_OP_e:
            gdImageArc (
                rip->u.image.img, opp->u.rect.o.x, opp->u.rect.o.y,
                2 * opp->u.rect.s.x, 2 * opp->u.rect.s.y,
                opp->u.rect.ba, opp->u.rect.ea, lnid
            );
            break;
        case RI_OP_P:
            gdImageFilledPolygon (
                rip->u.image.img, &rip->points[opp->u.poly.pi],
                opp->u.poly.pn, flid
            );
            break;
        case RI_OP_p:
            gdImagePolygon (
                rip->u.image.img, &rip->points[opp->u.poly.pi],
                opp->u.poly.pn, lnid
            );
            break;
        case RI_OP_L:
            gdImageOpenPolygon (
                rip->u.image.img, &rip->points[opp->u.poly.pi],
                opp->u.poly.pn, lnid
            );
            break;
        case RI_OP_b:
            if (gensplines (
                rip, &rip->points[opp->u.poly.pi], opp->u.poly.pn
            ) == -1) {
                SUwarning (0, "RIexecimageop", "cannot generate splines");
                return -1;
            }
            gdImageFilledPolygon (
                rip->u.image.img, &rip->sppoints[0], rip->sppointm, flid
            );
            break;
        case RI_OP_B:
            if (gensplines (
                rip, &rip->points[opp->u.poly.pi], opp->u.poly.pn
            ) == -1) {
                SUwarning (0, "RIexecimageop", "cannot generate splines");
                return -1;
            }
            gdImageOpenPolygon (
                rip->u.image.img, &rip->sppoints[0], rip->sppointm, lnid
            );
            break;
        case RI_OP_T:
            if (font.type == RI_FONT_TTF) {
                x = opp->u.text.p.x, y = opp->u.text.p.y;
                for (s1 = opp->u.text.t; s1; s1 = s2) {
                    s2 = strstr (s1, "\\n");
                    if (s2)
                        c = *s2, *s2 = 0;
                    if (opp->u.text.jx != -1 && (s3 = gdImageStringFT (
                        NULL, &r[0], 0, font.file, font.size, 0.0, 0.0, 0.0, s1
                    ))) {
                        SUwarning (
                            0, "RIexecimageop", "cannot size text %s/%d/%s/%s",
                            font.file, font.size, s1, s3
                        );
                        break;
                    }
                    dx = 0;
                    switch (opp->u.text.jx) {
                    case 0: dx = -(r[4] - r[0]) / 2; break;
                    case 1: dx = -(r[4] - r[0]);     break;
                    }
                    if (opp->u.text.jy != 2 && (s3 = gdImageStringFT (
                        NULL, &r[0], 0, font.file, font.size,
                        0.0, 0.0, 0.0, "Wy"
                    ))) {
                        SUwarning (
                            0, "RIexecimageop", "cannot size text %s/%d/%s/%s",
                            font.file, font.size, "Wy", s3
                        );
                        break;
                    }
                    dy = 0;
                    switch (opp->u.text.jy) {
                    case -1: dy = -r[5];              break;
                    case 0:  dy = -(r[1] + r[5]) / 2; break;
                    case 1:  dy = -r[1];              break;
                    }
                    if ((s3 = gdImageStringFT (
                        rip->u.image.img, NULL, fnid, font.file, font.size, 0.0,
                        x + dx, y + dy + 1, s1
                    ))) {
                        SUwarning (
                            0, "RIexecimageop", "cannot size text %s/%d/%s/%s",
                            font.file, font.size, s1, s3
                        );
                        break;
                    }
                    y += (r[1] - r[5]) + 2;
                    if (s2)
                        *s2 = c, s2 += 2;
                }
            } else {
                x = opp->u.text.p.x, y = opp->u.text.p.y;
                for (s1 = opp->u.text.t; s1; s1 = s2) {
                    s2 = strstr (s1, "\\n");
                    if (s2)
                        c = *s2, *s2 = 0;
                    l1 = strlen (s1);
                    dx = 0;
                    switch (opp->u.text.jx) {
                    case 0: dx = -(l1 * font.font->w) / 2; break;
                    case 1: dx = -(l1 * font.font->w);     break;
                    }
                    dy = 0;
                    switch (opp->u.text.jy) {
                    case -1: dy = 0;                   break;
                    case 0:  dy = -(font.font->h / 2); break;
                    case 1:  dy = -font.font->h;       break;
                    }
                    gdImageString (
                        rip->u.image.img, font.font, x + dx, y + dy + 1,
                        (unsigned char *) s1, fnid
                    );
                    y += font.font->h + 2;
                    if (s2)
                        *s2 = c, s2 += 2;
                }
            }
            break;
        case RI_OP_F:
            if (opp->u.font.f.type == RI_FONT_NONE)
                findfont (opp->u.font.t, opp->u.font.s, &font);
            else
                font = opp->u.font.f;
            break;
        case RI_OP_C:
            flid = parsecolor (rip, opp->u.color.t);
            break;
        case RI_OP_c:
            lnid = parsecolor (rip, opp->u.color.t);
            if (stn > 0) {
                for (sti = 0; sti < stn; sti++)
                    if (sts[sti] != gdTransparent)
                        sts[sti] = lnid;
                gdImageSetStyle (rip->u.image.img, sts, stn);
                lnid = gdStyled;
            }
            break;
        case RI_OP_FC:
            fnid = parsecolor (rip, opp->u.color.t);
            break;
        case RI_OP_S:
            if (opp->u.style.style != RI_STYLE_NONE) {
                stn = 0;
                if (opp->u.style.style == RI_STYLE_DOTTED) {
                    sts[0] = lnid, sts[2] = gdTransparent;
                    sts[1] = lnid, sts[3] = gdTransparent;
                    stn = 4;
                } else if (opp->u.style.style == RI_STYLE_DASHED) {
                    sts[0] = lnid, sts[4] = gdTransparent;
                    sts[1] = lnid, sts[5] = gdTransparent;
                    sts[2] = lnid, sts[6] = gdTransparent;
                    sts[3] = lnid, sts[7] = gdTransparent;
                    stn = 8;
                } else if (opp->u.style.style == RI_STYLE_LINEWIDTH) {
                    gdImageSetThickness (
                        rip->u.image.img, atoi (opp->u.style.t)
                    );
                }
                if (stn > 0) {
                    gdImageSetStyle (rip->u.image.img, sts, stn);
                    lnid = gdStyled;
                }
            }
            break;
        case RI_OP_I:
            gdImageCopy (
                rip->u.image.img, opp->u.img.img,
                opp->u.img.rx, opp->u.img.ry,
                opp->u.img.ix, opp->u.img.iy, opp->u.img.iw, opp->u.img.ih
            );
            break;
        case RI_OP_POS:
            break;
        case RI_OP_SIZ:
            break;
        case RI_OP_LW:
            gdImageSetThickness (rip->u.image.img, opp->u.length.l);
            break;
        case RI_OP_CLIP:
            gdImageSetClip (
                rip->u.image.img, opp->u.rect.o.x, opp->u.rect.o.y,
                opp->u.rect.o.x + opp->u.rect.s.x,
                opp->u.rect.o.y + opp->u.rect.s.y
            );
            break;
        }
    }
    return 0;
}

static int RIexeccanvasop (
    RI_t *rip, RIop_t *ops, int opm,
    char *lncolor, char *flcolor, char *fncolor,
    char *fnname, char *fnsize
) {
    RIop_t *opp;
    int opi, opj;
    int pi, fnsz;
    int x1, y1, x2, y2, cx1, cy1, cx2, cy2;
    char *s1, *s2, c;
    FILE *fp;
    gdImage *img;

    if (rip->cflag) {
        for (opi = 0; opi < opm; opi++) {
            opp = &ops[opi];
            if (
                opp->type != RI_OP_COPYC && opp->type != RI_OP_COPYc &&
                opp->type != RI_OP_COPYFC
            )
                continue;
            if (opp->type == RI_OP_COPYc)
                lncolor = opp->u.color.t;
            else if (opp->type == RI_OP_COPYC)
                flcolor = opp->u.color.t;
            else if (opp->type == RI_OP_COPYFC)
                fncolor = opp->u.color.t;
            for (opj = 0; opj < opm; opj++) {
                if (
                    (opp->type == RI_OP_COPYC && ops[opj].type == RI_OP_C) ||
                    (opp->type == RI_OP_COPYc && ops[opj].type == RI_OP_c) ||
                    (opp->type == RI_OP_COPYFC && ops[opj].type == RI_OP_FC)
                )
                    ops[opj].u.color.t = opp->u.color.t;
            }
        }
    }

    fnsz = atoi (fnsize);

    for (opi = 0; opi < opm; opi++) {
        opp = &ops[opi];
        switch (opp->type) {
        case RI_OP_E:
        case RI_OP_e:
            x1 = opp->u.rect.o.x - opp->u.rect.s.x;
            y1 = opp->u.rect.o.y;
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.beginPath ()\n"
                "  cntx.%sStyle = '%s'\n"
                "  cntx.moveTo (%d, %d)\n",
                (opp->type == RI_OP_E) ? "fill" : "stroke",
                (opp->type == RI_OP_E) ? flcolor : lncolor, x1, y1
            );
            cx1 = opp->u.rect.o.x - opp->u.rect.s.x;
            cy1 = opp->u.rect.o.y + B2E_CONST * opp->u.rect.s.y;
            cx2 = opp->u.rect.o.x - B2E_CONST * opp->u.rect.s.x;
            cy2 = opp->u.rect.o.y + opp->u.rect.s.y;
            x2 = opp->u.rect.o.x;
            y2 = opp->u.rect.o.y + opp->u.rect.s.y;
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.bezierCurveTo (%d, %d, %d, %d, %d, %d)\n",
                cx1, cy1, cx2, cy2, x2, y2
            );
            cx1 = opp->u.rect.o.x + B2E_CONST * opp->u.rect.s.x;
            cy1 = opp->u.rect.o.y + opp->u.rect.s.y;
            cx2 = opp->u.rect.o.x + opp->u.rect.s.x;
            cy2 = opp->u.rect.o.y + B2E_CONST * opp->u.rect.s.y;
            x2 = opp->u.rect.o.x + opp->u.rect.s.x;
            y2 = opp->u.rect.o.y;
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.bezierCurveTo (%d, %d, %d, %d, %d, %d)\n",
                cx1, cy1, cx2, cy2, x2, y2
            );
            cx1 = opp->u.rect.o.x + opp->u.rect.s.x;
            cy1 = opp->u.rect.o.y - B2E_CONST * opp->u.rect.s.y;
            cx2 = opp->u.rect.o.x + B2E_CONST * opp->u.rect.s.x;
            cy2 = opp->u.rect.o.y - opp->u.rect.s.y;
            x2 = opp->u.rect.o.x;
            y2 = opp->u.rect.o.y - opp->u.rect.s.y;
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.bezierCurveTo (%d, %d, %d, %d, %d, %d)\n",
                cx1, cy1, cx2, cy2, x2, y2
            );
            cx1 = opp->u.rect.o.x - B2E_CONST * opp->u.rect.s.x;
            cy1 = opp->u.rect.o.y - opp->u.rect.s.y;
            cx2 = opp->u.rect.o.x;
            cy2 = opp->u.rect.o.y - B2E_CONST * opp->u.rect.s.y;
            x2 = opp->u.rect.o.x - opp->u.rect.s.x;
            y2 = opp->u.rect.o.y;
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.bezierCurveTo (%d, %d, %d, %d, %d, %d)\n"
                "  cntx.%s ()\n",
                cx1, cy1, cx2, cy2, x2, y2,
                (opp->type == RI_OP_E) ? "fill" : "stroke"
            );
            break;
        case RI_OP_P:
        case RI_OP_p:
        case RI_OP_L:
            x1 = rip->points[opp->u.poly.pi].x;
            y1 = rip->points[opp->u.poly.pi].y;
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.beginPath ()\n"
                "  cntx.%sStyle = '%s'\n"
                "  cntx.moveTo (%d, %d)\n",
                (opp->type == RI_OP_P) ? "fill" : "stroke",
                (opp->type == RI_OP_P) ? flcolor : lncolor, x1, y1
            );
            for (pi = 1; pi < opp->u.poly.pn; pi++) {
                x2 = rip->points[opp->u.poly.pi + pi].x;
                y2 = rip->points[opp->u.poly.pi + pi].y;
                sfprintf (
                    rip->u.canvas.fp,
                    "  cntx.lineTo (%d, %d)\n",
                    x2, y2
                );
            }
            if (opp->type == RI_OP_p)
                sfprintf (
                    rip->u.canvas.fp,
                    "  cntx.closePath ()\n"
                );
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.%s ()\n",
                (opp->type == RI_OP_P) ? "fill" : "stroke"
            );
            break;
        case RI_OP_b:
        case RI_OP_B:
            x1 = rip->points[opp->u.poly.pi].x;
            y1 = rip->points[opp->u.poly.pi].y;
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.beginPath ()\n"
                "  cntx.%sStyle = '%s'\n"
                "  cntx.moveTo (%d, %d)\n",
                (opp->type == RI_OP_b) ? "fill" : "stroke",
                (opp->type == RI_OP_b) ? flcolor : lncolor, x1, y1
            );
            for (pi = 0; pi < opp->u.poly.pn - 1; pi += 3) {
                cx1 = rip->points[opp->u.poly.pi + pi + 1].x;
                cy1 = rip->points[opp->u.poly.pi + pi + 1].y;
                cx2 = rip->points[opp->u.poly.pi + pi + 2].x;
                cy2 = rip->points[opp->u.poly.pi + pi + 2].y;
                x2 = rip->points[opp->u.poly.pi + pi + 3].x;
                y2 = rip->points[opp->u.poly.pi + pi + 3].y;
                sfprintf (
                    rip->u.canvas.fp,
                    "  cntx.bezierCurveTo (%d, %d, %d, %d, %d, %d)\n",
                    cx1, cy1, cx2, cy2, x2, y2
                );
            }
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.%s ()\n",
                (opp->type == RI_OP_b) ? "fill" : "stroke"
            );
            break;
        case RI_OP_T:
            x1 = opp->u.text.p.x, y1 = opp->u.text.p.y;
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.beginPath ()\n"
                "  cntx.font = '%s %dpx'\n"
                "  cntx.textAlign = '%s'\n",
                fnname, fnsz,
                (opp->u.text.jx == -1) ? "left" : (
                    (opp->u.text.jx == 0) ? "center": "right"
                )
            );
            for (s1 = opp->u.text.t; s1; s1 = s2) {
                s2 = strstr (s1, "\\n");
                if (s2)
                    c = *s2, *s2 = 0;
                sfprintf (
                    rip->u.canvas.fp,
                    "  cntx.strokeText ('%s', %d, %d)\n",
                    s1, x1, y1
                );
                y1 += fnsz;
                if (s2)
                    *s2 = c, s2 += 2;
            }
            break;
        case RI_OP_F:
            fnname = opp->u.font.t;
            fnsz = opp->u.font.s;
            break;
        case RI_OP_C:
            flcolor = opp->u.color.t;
            break;
        case RI_OP_c:
            lncolor = opp->u.color.t;
            break;
        case RI_OP_FC:
            fncolor = opp->u.color.t;
            break;
        case RI_OP_S:
            break;
        case RI_OP_I:
            if (!(fp = fopen (sfprints (
                "%s.%d.%d.gif", rip->fprefix, rip->findex, imageid
            ), "w"))) {
                SUwarning (
                    0, "RIexeccanvasop", "cannot create file %s.%d.%d.gif",
                    rip->fprefix, rip->findex, imageid
                );
                return -1;
            }
            if (!(img = gdImageCreateTrueColor (
                opp->u.img.iw, opp->u.img.ih
            ))) {
                SUwarning (
                    0, "RIexeccanvasop", "cannot create tmp output image"
                );
                return -1;
            }
            gdImageCopy (
                img, opp->u.img.img, 0, 0,
                opp->u.img.ix, opp->u.img.iy, opp->u.img.iw, opp->u.img.ih
            );
            gdImageGif (img, fp);
            gdImageDestroy (img);
            fclose (fp);
            sfprintf (
                rip->u.canvas.fp,
                "  var img%d = new Image ()\n"
                "  img%d.src = '%s%s.%d.%d.gif'\n"
                "  cntx.drawImage (img%d, %d, %d)\n",
                imageid, imageid, imageprefix, rip->fprefix, rip->findex,
                imageid, imageid, opp->u.img.rx, opp->u.img.ry
            );
            imageid++;
            break;
        case RI_OP_POS:
            break;
        case RI_OP_SIZ:
            break;
        case RI_OP_LW:
            sfprintf (
                rip->u.canvas.fp,
                "  cntx.lineWidth = %d\n",
                opp->u.length.l
            );
            break;
        case RI_OP_CLIP:
            break;
        }
    }
    return 0;
}

static int RIexectableop (
    RI_t *rip, RIop_t *ops, int opm,
    char *bgcolor, char *color, char *fnname, char *fnsize
) {
    RIop_t *opp;
    int opi;
    char *jstr, *trclass, *s1, *s2, c;
    int fnsz;
    FILE *fp;
    gdImage *img;

    trclass = NULL;
    fnsz = atoi (fnsize);
    for (opi = 0; opi < opm; opi++) {
        opp = &ops[opi];
        switch (opp->type) {
        case RI_OP_HTMLTBB:
            sfprintf (rip->u.table.fp, "<table class='%s'>\n", opp->u.htb.tid);
            break;
        case RI_OP_HTMLTBCB:
            sfprintf (rip->u.table.fp, "<caption class='%s'>", opp->u.htb.tid);
            if (opp->u.htb.lab)
                sfprintf (rip->u.table.fp, "%s", opp->u.htb.lab);
            break;
        case RI_OP_HTMLTBCE:
            sfprintf (rip->u.table.fp, "</caption>\n");
            break;
        case RI_OP_HTMLTBE:
            sfprintf (rip->u.table.fp, "</table>\n");
            break;
        case RI_OP_HTMLTRB:
            sfprintf (rip->u.table.fp, "<tr class='%s'>\n", opp->u.htr.tid);
            trclass = opp->u.htr.tid;
            break;
        case RI_OP_HTMLTRE:
            sfprintf (rip->u.table.fp, "</tr>\n");
            break;
        case RI_OP_HTMLTDB:
            sfprintf (
                rip->u.table.fp, "<td class='%s' style='", opp->u.htd.tid
            );
            switch (opp->u.htd.j) {
            case -1: jstr = "left";   break;
            case 0:  jstr = "center"; break;
            case 1:  jstr = "right";  break;
            }
            sfprintf (rip->u.table.fp, "text-align:%s'", jstr);
            if (opp->u.htd.cn > 1)
                sfprintf (rip->u.table.fp, " colspan=%d", opp->u.htd.cn);
            sfprintf (rip->u.table.fp, ">");
            break;
        case RI_OP_HTMLTDT:
            for (s1 = opp->u.htd.lab; s1 && s1[0]; s1 = s2) {
                s2 = strstr (s1, "_INHERIT_TR_CLASS_");
                if (s2)
                    c = *s2, *s2 = 0;
                sfprintf (rip->u.table.fp, "%s", s1);
                if (s2) {
                    sfprintf (rip->u.table.fp, "'%s'", trclass ? trclass : "");
                    *s2 = c, s2 += 18;
                }
            }
            break;
        case RI_OP_HTMLTDE:
            sfprintf (rip->u.table.fp, "</td>\n");
            break;
        case RI_OP_HTMLAB:
            sfprintf (
                rip->u.table.fp,
                "<a class='%s' href='%s' %s%s%s%s>",
                opp->u.ha.tid, opp->u.ha.url,
                opp->u.ha.url ? "" : "style='text-decoration:none'",
                opp->u.ha.info ? "title='" : "",
                opp->u.ha.info ? opp->u.ha.info : "",
                opp->u.ha.info ? "'" : ""
            );
            break;
        case RI_OP_HTMLAE:
            sfprintf (rip->u.table.fp, "</a>");
            break;
        case RI_OP_F:
            fnname = opp->u.font.t;
            fnsz = opp->u.font.s;
            break;
        case RI_OP_FC:
            color = opp->u.color.t;
            break;
        case RI_OP_T:
            for (s1 = opp->u.text.t; s1; s1 = s2) {
                s2 = strstr (s1, "\\n");
                if (s2)
                    c = *s2, *s2 = 0;
                sfprintf (rip->u.table.fp, "<span style='");
                if (fnname)
                    sfprintf (rip->u.table.fp, "font-family:%s;", fnname);
                if (fnsz >= 0)
                    sfprintf (rip->u.table.fp, "font-size:%d;", fnsz);
                if (color)
                    sfprintf (rip->u.table.fp, "color:%s;", color);
                sfprintf (rip->u.table.fp, "'>\n");
                sfprintf (rip->u.table.fp, "%s</span>", s1);
                if (s2)
                    *s2 = c, s2 += 2;
            }
            break;
        case RI_OP_I:
            if (!(fp = fopen (sfprints (
                "%s.%d.%d.gif", rip->fprefix, rip->findex, imageid
            ), "w"))) {
                SUwarning (
                    0, "RIexectableop", "cannot create file %s.%d.%d.gif",
                    rip->fprefix, rip->findex, imageid
                );
                return -1;
            }
            if (!(img = gdImageCreateTrueColor (
                opp->u.img.iw, opp->u.img.ih
            ))) {
                SUwarning (
                    0, "RIexectableop", "cannot create tmp output image"
                );
                return -1;
            }
            gdImageCopy (
                img, opp->u.img.img, 0, 0,
                opp->u.img.ix, opp->u.img.iy, opp->u.img.iw, opp->u.img.ih
            );
            gdImageGif (img, fp);
            gdImageDestroy (img);
            fclose (fp);
            sfprintf (
                rip->u.table.fp, "<img src='%s%s.%d.%d.gif'>",
                imageprefix, rip->fprefix, rip->findex, imageid
            );
            imageid++;
            break;
        case RI_OP_HTMLCOM:
            sfprintf (rip->u.table.fp, "<!--%s-->\n", opp->u.hcom.str);
            break;
        }
    }
    return 0;
}

int RIloadimage (char *file, RIop_t *opp) {
    FILE *fp;

    if (!(fp = fopen (file, "r"))) {
        SUwarning (1, "RIloadimage", "cannot open file %s", file);
        return 0;
    }
    if (!(opp->u.img.img = gdImageCreateFromGif (fp))) {
        SUwarning (1, "RIloadimage", "cannot read file %s", file);
        return 0;
    }
    opp->u.img.rx = 0, opp->u.img.ry = 0;
    opp->u.img.ix = 0, opp->u.img.iy = 0;
    opp->u.img.iw = gdImageSX (opp->u.img.img);
    opp->u.img.ih = gdImageSY (opp->u.img.img);
    opp->type = RI_OP_I;
    fclose (fp);
    return 0;
}

static int parsecolor (RI_t *rip, char *name) {
    RIcolor_t *cp, *cmem;
    colormap_t *cmp;
    int cmi;
    unsigned long rgb;
    char *s;

    if (!(cp = dtmatch (rip->cdict, name))) {
        if (!(cmem = vmalloc (rip->vm, sizeof (RIcolor_t)))) {
            SUwarning (0, "parsecolor", "cannot allocate cmem");
            return -1;
        }
        if (!(cmem->name = vmstrdup (rip->vm, name))) {
            SUwarning (0, "parsecolor", "cannot copy name");
            return -1;
        }
        if ((cp = dtinsert (rip->cdict, cmem)) != cmem) {
            SUwarning (0, "parsecolor", "cannot insert color");
            return -1;
        }
        cp->id = -1;
        for (cmi = 0; colormaps[cmi].name; cmi++) {
            if (strcasecmp (name, colormaps[cmi].name) == 0) {
                cmp = &colormaps[cmi];
                cp->id = gdImageColorAllocate (
                    rip->u.image.img, cmp->r, cmp->g, cmp->b
                );
                break;
            }
        }
        if (cp->id == -1) {
            if (name && name[0] == '#' && strlen (name) == 7) {
                rgb = strtoul (name + 1, &s, 16);
                cp->id = gdImageColorAllocate (
                    rip->u.image.img,
                    rgb >> 16 & 0xFF, rgb >> 8 & 0xFF, rgb & 0xFF
                );
            }
        }
        if (cp->id == -1)
            cp->id = rip->u.image.fgid;
    }
    return cp->id;
}

static int findfont (char *name, int size, RIfont_t *fp) {
    int fonti;
    char *s;

    for (fonti = 0; fontmap[fonti][0]; fonti++)
        if (strncasecmp (
            name, fontmap[fonti][0], strlen (fontmap[fonti][0])
        ) == 0)
            break;
    if (fontmap[fonti][0] && (
        s = pathpath (fontmap[fonti][1], "", 0, NULL, 0)
    )) {
        memset (fp, 0, sizeof (RIfont_t));
        fp->type = RI_FONT_TTF;
        fp->size = size;
        fp->file = s;
        return 0;
    }

    for (fonti = 0; fonti < RI_FONT_SIZE; fonti++)
        if (fonti == RI_FONT_SIZE - 1 || fonts[fonti + 1].size > size * 1.3)
            break;
    if (fonti == RI_FONT_SIZE)
        fonti = RI_FONT_SIZE - 1;
    memcpy (fp, &fonts[fonti], sizeof (RIfont_t));
    return 0;
}

static int gensplines (RI_t *rip, RIpoint_t *points, int pointn) {
    int pointi, pointm;
    RIpoint_t p0, p1, p2, p3, gp0, gp1, gp2, s;
    double t;
    int n, i;

    rip->sppointm = 0;
    for (pointi = 0; pointi < pointn - 1; pointi += 3) {
        p0 = points[pointi + 0], p1 = points[pointi + 1];
        p2 = points[pointi + 2], p3 = points[pointi + 3];
        if ((s.x = p3.x - p0.x) < 0)
            s.x = - s.x;
        if ((s.y = p3.y - p0.y) < 0)
            s.y = - s.y;
        if (s.x > s.y)
            n = s.x / 5 + 1;
        else
            n = s.y / 5 + 1;
        pointm = n + 1;
        if (rip->sppointm + pointm >= rip->sppointn) {
            if (!(rip->sppoints = vmresize (
                rip->vm, rip->sppoints,
                (rip->sppointn + pointm) * sizeof (RIpoint_t), VM_RSCOPY
            ))) {
                SUwarning (0, "gensplines", "cannot grow point array");
                return -1;
            }
            rip->sppointn += pointm;
        }
        for (i = 0; i <= n; i++) {
            t = i / (double) n;
            gp0.x = p0.x + t * (p1.x - p0.x);
            gp0.y = p0.y + t * (p1.y - p0.y);
            gp1.x = p1.x + t * (p2.x - p1.x);
            gp1.y = p1.y + t * (p2.y - p1.y);
            gp2.x = p2.x + t * (p3.x - p2.x);
            gp2.y = p2.y + t * (p3.y - p2.y);
            gp0.x = gp0.x + t * (gp1.x - gp0.x);
            gp0.y = gp0.y + t * (gp1.y - gp0.y);
            gp1.x = gp1.x + t * (gp2.x - gp1.x);
            gp1.y = gp1.y + t * (gp2.y - gp1.y);
            rip->sppoints[rip->sppointm].x = gp0.x + t * (gp1.x - gp0.x) + 0.5;
            rip->sppoints[rip->sppointm].y = gp0.y + t * (gp1.y - gp0.y) + 0.5;
            rip->sppointm++;
        }
    }
    return 0;
}

static char cnvbuf[10240];

static char *htmlreenc (char *istr) {
    char *is, *os;

    for (is = istr, os = cnvbuf; *is; is++) {
        if (os - cnvbuf + 4 > 10240)
            return istr;
        if (*is == '\\' && *(is + 1) == 'n')
            *os++ = '<', *os++ = 'b', *os++ = 'r', *os++ = '>', is++;
        else
            *os++ = *is;
    }
    *os = 0;
    return cnvbuf;
}

static char *nlreenc (char *istr) {
    char *is, *os;

    for (is = istr, os = cnvbuf; *is; is++) {
        if (os - cnvbuf + 4 > 10240)
            return istr;
        if (*is == '\\' && *(is + 1) == 'n')
            *os++ = '\n', is++;
        else
            *os++ = *is;
    }
    *os = 0;
    return cnvbuf;
}

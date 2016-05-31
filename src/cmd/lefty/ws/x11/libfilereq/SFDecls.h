/* SFDecls.h */

/* We don't rely on compiler doing the right thing
 * in absence of declarations.
 * C compilers should never have accepted this braindamage.
 * KG <garloff@suse.de>, 2002-01-28
 */

/* SelFile.c */
FILE * SFopenFile (char *, char *, char *, char *);
void SFtextChanged (void);

/* Draw.c */
void SFinitFont (void);
void SFcreateGC (void);
void SFclearList (int, int);
void SFdrawList (int, int);
void SFdrawLists (int);

/* Path.c */
int SFchdir (char *path);
void SFupdatePath (void);
void SFsetText (char *path);

/* Dir.c */
int SFcompareEntries (const void *vp, const void *vq);
int SFgetDir (SFDir *dir);

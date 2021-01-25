#ifndef CLUSTI_H
#define CLUSTI_H

/* -------------------------------------------------------------------------- */
/* Type forward decls. */

struct Clusti_Stitcher;
typedef struct Clusti_Stitcher Clusti_Stitcher;

/* -------------------------------------------------------------------------- */
/* Function API */

/**
 * @brief Constructor
 * @return Clusti_Stitcher with all members set to zero
*/
Clusti_Stitcher *clusti_Stitcher_create();
/**
 * @brief Destructor
*/
void clusti_Stitcher_destroy(Clusti_Stitcher *stitcher);

void clusti_Stitcher_readConfig(Clusti_Stitcher *stitcher,
				char const *configPath);

/* -------------------------------------------------------------------------- */

#endif /* CLUSTI_H */

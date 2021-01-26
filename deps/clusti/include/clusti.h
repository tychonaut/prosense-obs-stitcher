#ifndef CLUSTI_H
#define CLUSTI_H

/* -------------------------------------------------------------------------- */
/* Type forward decls. */

struct Clusti;
typedef struct Clusti Clusti;

/* -------------------------------------------------------------------------- */
/* Function API */

/**
 * @brief Constructor
 * @return Clusti_Stitcher with all members set to zero
*/
Clusti *clusti_create();
/**
 * @brief Destructor
*/
void clusti_destroy(Clusti *instance);

void clusti_readConfig(Clusti *instance,
				char const *configPath);

/* -------------------------------------------------------------------------- */

#endif /* CLUSTI_H */

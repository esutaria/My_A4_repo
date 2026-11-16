/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Eesha Sutaria, Maria Wiik                                  */
/*--------------------------------------------------------------------*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "path.h"
#include "nodeFT.h"
#include "checkerFT.h"
#include "ft.h"

/*
 A File Tree is a representation of a hierarchy of directories and/or files,
 represented as an AO with 3 state variables:
*/

/* 1. a flag for being in an initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. a pointer to the root node in the hierarchy */
static Node_T oNRoot;
/* 3. a counter of the number of nodes in the hierarchy */
static size_t ulCount;

/* --------------------------------------------------------------------




 The FT_traversePath and FT_findNode functions modularize the common
 functionality of going as far as possible down an FT towards a path
 and returning either the node of however far was reached or the
 node if the full path was reached, respectively.
*/

/*
 Traverses the FT starting at the root as far as possible towards
 absolute path oPPath. If able to traverse, returns an int SUCCESS
 status and sets *poNFurthest to the furthest node reached (which may
 be only a prefix of oPPath, or even NULL if the root is NULL).
 Otherwise, sets *poNFurthest to NULL and returns with status:
 * CONFLICTING_PATH if the root's path is not a prefix of oPPath
 * MEMORY_ERROR if memory could not be allocated to complete request
*/
static int FT_traversePath(Path_T oPPath, Node_T *poNFurthest)
{
    int iStatus;
    Path_T oPPrefix = NULL;
    Node_T oNCurr;
    Node_T oNChild = NULL;
    size_t ulDepth;
    size_t i;
    size_t ulChildID = 0;

    assert(oPPath != NULL);
    assert(poNFurthest != NULL);

    /* root is NULL -> won't find anything */
    if (oNRoot == NULL)
    {
        *poNFurthest = NULL;
        return SUCCESS;
    }

    iStatus = Path_prefix(oPPath, 1, &oPPrefix);
    if (iStatus != SUCCESS)
    {
        *poNFurthest = NULL;
        return iStatus;
    }

    if (Path_comparePath(Node_getPath(oNRoot), oPPrefix))
    {
        Path_free(oPPrefix);
        *poNFurthest = NULL;
        return CONFLICTING_PATH;
    }
    Path_free(oPPrefix);
    oPPrefix = NULL;

    oNCurr = oNRoot;
    ulDepth = Path_getDepth(oPPath);
    for (i = 2; i <= ulDepth; i++)
    {
        iStatus = Path_prefix(oPPath, i, &oPPrefix);
        if (iStatus != SUCCESS)
        {
            *poNFurthest = NULL;
            return iStatus;
        }
        if (Node_hasChild(oNCurr, oPPrefix, &ulChildID))
        {
            /* go to that child and continue with next prefix */
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = Node_getChild(oNCurr, ulChildID, &oNChild);
            if (iStatus != SUCCESS)
            {
                *poNFurthest = NULL;
                return iStatus;
            }
            oNCurr = oNChild;
        }
        else
        {
            /* oNCurr doesn't have child with path oPPrefix:
               this is as far as we can go */
            break;
        }
    }

    Path_free(oPPrefix);
    *poNFurthest = oNCurr;
    return SUCCESS;
}

/*
 Traverses the FT to find a node with absolute path pcPath. Returns a
 int SUCCESS status and sets *poNResult to be the node, if found.
 Otherwise, sets *poNResult to NULL and returns with status:
 * INITIALIZATION_ERROR if the FT is not in an initialized state
 * BAD_PATH if pcPath does not represent a well-formatted path
 * CONFLICTING_PATH if the root's path is not a prefix of pcPath
 * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
 * MEMORY_ERROR if memory could not be allocated to complete request
*/
static int FT_findNode(const char *pcPath, Node_T *poNResult)
{
    Path_T oPPath = NULL;
    Node_T oNFound = NULL;
    int iStatus;

    assert(pcPath != NULL);
    assert(poNResult != NULL);

    if (!bIsInitialized)
    {
        *poNResult = NULL;
        return INITIALIZATION_ERROR;
    }

    iStatus = Path_new(pcPath, &oPPath);
    if (iStatus != SUCCESS)
    {
        *poNResult = NULL;
        return iStatus;
    }

    iStatus = FT_traversePath(oPPath, &oNFound);
    if (iStatus != SUCCESS)
    {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }

    if (oNFound == NULL)
    {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    if (Path_comparePath(Node_getPath(oNFound), oPPath) != 0)
    {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    Path_free(oPPath);
    *poNResult = oNFound;
    return SUCCESS;
}

/*
 Inserts a new node with absolute path pcPath into the FT, creating
 any missing ancestor directories. The last node is a file holding
 pvContents of length ulLength if isFile is TRUE, otherwise it is a
 directory (pvContents and ulLength are then ignored). Returns
 SUCCESS, or one of the statuses listed for FT_insertDir and
 FT_insertFile in ft.h.
*/
static int FT_insertPath(const char *pcPath, boolean isFile,
                         void *pvContents, size_t ulLength)
{
    int iStatus;
    Path_T oPPath = NULL;
    Node_T oNFirstNew = NULL;
    Node_T oNCurr = NULL;
    size_t ulDepth, ulIndex;
    size_t ulNewNodes = 0;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    /* validate pcPath and generate a Path_T for it */
    if (!bIsInitialized)
        return INITIALIZATION_ERROR;

    iStatus = Path_new(pcPath, &oPPath);
    if (iStatus != SUCCESS)
        return iStatus;

    /* find the closest ancestor of oPPath already in the tree */
    iStatus = FT_traversePath(oPPath, &oNCurr);
    if (iStatus != SUCCESS)
    {
        Path_free(oPPath);
        return iStatus;
    }

    /* no ancestor node found, so if root is not NULL,
       pcPath isn't underneath root. */
    if (oNCurr == NULL && oNRoot != NULL)
    {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    ulDepth = Path_getDepth(oPPath);
    if (oNCurr == NULL) /* new root! */
        ulIndex = 1;
    else
    {
        ulIndex = Path_getDepth(Node_getPath(oNCurr)) + 1;

        /* oNCurr is the node we're trying to insert, as a dir or file */
        if (ulIndex == ulDepth + 1 && !Path_comparePath(oPPath,
                                                        Node_getPath(oNCurr)))
        {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }

        /* the path continues below oNCurr, but oNCurr is a file */
        if (Node_isFile(oNCurr))
        {
            Path_free(oPPath);
            return NOT_A_DIRECTORY;
        }
    }

    /* starting at oNCurr, build rest of the path one level at a time */
    while (ulIndex <= ulDepth)
    {
        Path_T oPPrefix = NULL;
        Node_T oNNewNode = NULL;

        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if (iStatus != SUCCESS)
        {
            Path_free(oPPath);
            if (oNFirstNew != NULL)
                (void)Node_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* insert the new node for this level: only the last level can
           be a file, every level before it is a directory */
        iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode,
                           (boolean)(isFile && ulIndex == ulDepth),
                           pvContents, ulLength);
        if (iStatus != SUCCESS)
        {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if (oNFirstNew != NULL)
                (void)Node_free(oNFirstNew);
            assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
            return iStatus;
        }

        /* set up for next level */
        Path_free(oPPrefix);
        oNCurr = oNNewNode;
        ulNewNodes++;
        if (oNFirstNew == NULL)
            oNFirstNew = oNCurr;
        ulIndex++;
    }

    Path_free(oPPath);
    /* update FT state variables to reflect insertion */
    if (oNRoot == NULL)
        oNRoot = oNFirstNew;
    ulCount += ulNewNodes;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

/* Function to insert directory based on pcPath (path given).
   Uses FT_insertPath. The way it is specific for a
   dir is that isFile = FALSE and there should be no
   pvContents (NULL) and the length of the contents
   should thus be 0 */
int FT_insertDir(const char *pcPath)
{
    return FT_insertPath(pcPath, FALSE, NULL, 0);
}

/* FT_insertFile works the same way as insertDir
   (by using the pcPath to traverse the tree
   until it find the most in depth node there
   and insert new nodes where they are needed).
   It adds the actual values for contents
   and the length of the contents - that is why
   we have the extra parameters (pvContents
   and ulLength).
*/
int FT_insertFile(const char *pcPath, void *pvContents,
                  size_t ulLength)
{
    return FT_insertPath(pcPath, TRUE, pvContents, ulLength);
}

/* FT_containsDir takes a path as input and checks
   whether that path can be found in the tree
   i.e. If there is a file with the same path,
   it will return FALSE
*/
boolean FT_containsDir(const char *pcPath)
{
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);
    if (iStatus != SUCCESS)
        return FALSE;

    return (boolean)!Node_isFile(oNFound);
}

/* Based on the path provided (pcPath),
check if the file is in the tree.
If there is a directory with the same path,
it will return FALSE
*/
boolean FT_containsFile(const char *pcPath)
{
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);
    if (iStatus != SUCCESS)
        return FALSE;

    return (boolean)Node_isFile(oNFound);
}

/* This function tries to remove the directory with path
pcPath. If there is a file with the same path, it will not
be removed.
*/
int FT_rmDir(const char *pcPath)
{
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS)
        return iStatus;

    /* a file cannot be removed as a directory */
    if (Node_isFile(oNFound))
        return NOT_A_DIRECTORY;

    ulCount -= Node_free(oNFound);
    if (ulCount == 0)
        oNRoot = NULL;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

/* This function tries to remove the file with path
pcPath. If there is a file with the same path, it will not
be removed.
*/
int FT_rmFile(const char *pcPath)
{
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS)
        return iStatus;

    /* a directory cannot be removed as a file */
    if (!Node_isFile(oNFound))
        return NOT_A_FILE;

    ulCount -= Node_free(oNFound);
    if (ulCount == 0)
        oNRoot = NULL;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

/* FT_getFileContents gets the file contents associated with
the path pcPath. It first checks if this path or node exist in
the tree. Then, it ensures the path does not correspond
to a dir (as those do not have contents).
*/
void *FT_getFileContents(const char *pcPath)
{
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS)
        return NULL;

    /* a directory cannot have file contents */
    if (!Node_isFile(oNFound))
        return NULL;

    return Node_getContents(oNFound);
}

/* FT_replaceFileContents replaces the file contents associated with
the path pcPath. It first checks if this path or node exist in
the tree. Then, it ensures the path does not correspond
to a dir (as those do not have contents).
It will modify the contents to have what pvNewContents stores,
and it will have a new length ulNewLength.
*/
void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength)
{
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    iStatus = FT_findNode(pcPath, &oNFound);

    if (iStatus != SUCCESS)
        return NULL;

    /* a directory cannot have file contents */
    if (!Node_isFile(oNFound))
        return NULL;

    return Node_replaceContents(oNFound, pvNewContents, ulNewLength);
}

/*
 Reports information about the node at absolute path pcPath.
 If the node is found and it is a file, pbIsFile = TRUE and
 pulSize = length of file contents (in bytes). If it's a dir,
 the only change is pbIsFile = FALSE.
*/
int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize){
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);
    assert(pbIsFile != NULL);
    assert(pulSize != NULL);

    iStatus = FT_findNode(pcPath, &oNFound);

    /* on any failure, leave both outputs unchanged */
    if (iStatus != SUCCESS)
        return iStatus;

    if (Node_isFile(oNFound))
    {
        *pbIsFile = TRUE;
        *pulSize = Node_getLength(oNFound);
    }
    else
    {
        /* for a directory, *pulSize stays unchanged */
        *pbIsFile = FALSE;
    }

    return SUCCESS;
}

/* Initialization of the tree. Sets bIsInitialized to TRUE to
ensure there will only ever be one tree
It does not yet create any nodes. */
int FT_init(void)
{
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    if (bIsInitialized)
        return INITIALIZATION_ERROR;

    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulCount = 0;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

/* This destroys the entire tree (all of its nodes)
   and frees each node */
int FT_destroy(void)
{
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    if (!bIsInitialized)
        return INITIALIZATION_ERROR;
    if (oNRoot)
    {
        ulCount -= Node_free(oNRoot);
        oNRoot = NULL;
    }

    bIsInitialized = FALSE;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

/* --------------------------------------------------------------------
 The following auxiliary functions are used for generating the
 string representation of the FT.
*/

/*
 Performs a pre-order traversal of the tree rooted at n,
 inserting each payload to DynArray_T d beginning at index i.
 Returns the next unused index in d after the insertion(s).
*/
static size_t FT_preOrderTraversal(Node_T n, DynArray_T d, size_t i)
{
    size_t c;
    int iStatus;
    Node_T oNChild = NULL;

    assert(d != NULL);

    if (n != NULL)
    {
        (void)DynArray_set(d, i, n);
        i++;

        /* first pass: file children only */
        for (c = 0; c < Node_getNumChildren(n); c++)
        {
            iStatus = Node_getChild(n, c, &oNChild);
            assert(iStatus == SUCCESS);
            if (Node_isFile(oNChild))
                i = FT_preOrderTraversal(oNChild, d, i);
        }

        /* second pass: directory children only */
        for (c = 0; c < Node_getNumChildren(n); c++)
        {
            iStatus = Node_getChild(n, c, &oNChild);
            assert(iStatus == SUCCESS);
            if (!Node_isFile(oNChild))
                i = FT_preOrderTraversal(oNChild, d, i);
        }
    }
    return i;
}

/*
 Alternate version of strlen that uses pulAcc as an in-out parameter
 to accumulate a string length, rather than returning the length of
 oNNode's path, and also always adds one addition byte to the sum.
*/
static void FT_strlenAccumulate(Node_T oNNode, size_t *pulAcc)
{
    assert(pulAcc != NULL);

    if (oNNode != NULL)
        *pulAcc += (Path_getStrLength(Node_getPath(oNNode)) + 1);
}

/*
 Alternate version of strcat that inverts the typical argument
 order, appending oNNode's path onto pcAcc, and also always adds one
 newline at the end of the concatenated string.
*/
static void FT_strcatAccumulate(Node_T oNNode, char *pcAcc)
{
    assert(pcAcc != NULL);

    if (oNNode != NULL)
    {
        strcat(pcAcc, Path_getPathname(Node_getPath(oNNode)));
        strcat(pcAcc, "\n");
    }
}
/* Provides string representation of the tree - all paths represented */
char *FT_toString(void)
{
    DynArray_T nodes;
    size_t totalStrlen = 1;
    char *result = NULL;

    if (!bIsInitialized)
        return NULL;

    nodes = DynArray_new(ulCount);
    if (nodes == NULL)
    {
        return NULL;
    }
    (void)FT_preOrderTraversal(oNRoot, nodes, 0);

    DynArray_map(nodes, (void (*)(void *, void *))FT_strlenAccumulate,
                 (void *)&totalStrlen);

    result = malloc(totalStrlen);
    if (result == NULL)
    {
        DynArray_free(nodes);
        return NULL;
    }
    *result = '\0';

    DynArray_map(nodes, (void (*)(void *, void *))FT_strcatAccumulate,
                 (void *)result);

    DynArray_free(nodes);
    return result;
}

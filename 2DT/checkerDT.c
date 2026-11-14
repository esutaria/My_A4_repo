/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"


/* Returns the number of nodes in the subtree rooted at oNNode. */
static size_t CheckerDT_countNodes(Node_T oNNode) {
   size_t ulIndex;
   size_t ulTotal = 1;
   int iStatus;
   Node_T oNChild = NULL;

   if(oNNode == NULL)
      return 0;

   for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++) {
      iStatus = Node_getChild(oNNode, ulIndex, &oNChild);
      if(iStatus == SUCCESS)
         ulTotal += CheckerDT_countNodes(oNChild);
   }
   return ulTotal;
}

/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;
   Path_T oPNPath;
   Path_T oPPPath;
   size_t i;
   int iStatus;
   int iCompare;
   Node_T oNChild = NULL;
   Node_T oNPrev = NULL;


   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if(oNParent != NULL) {
      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }


for(i = 0; i < Node_getNumChildren(oNNode); i++) {
   iStatus = Node_getChild(oNNode, i, &oNChild);
   if(iStatus != SUCCESS) {
      fprintf(stderr, "Could not get a child within range\n");
      return FALSE;
   }


   /* children must be in increasing path order because a binary search is used when checking if something is already in the tree*/
   if(i > 0) {
      iCompare = Path_comparePath(Node_getPath(oNPrev),
                                  Node_getPath(oNChild));
      if(iCompare == 0) {
         fprintf(stderr, "A node has two children with the same path\n");
         return FALSE;
      }
      if(iCompare > 0) {
         fprintf(stderr, "A node's children are not in order\n");
         return FALSE;
      }
   }

   oNPrev = oNChild;
}

   return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

   You may want to change this function's return type or
   parameter list to facilitate constructing your checks.
   If you do, you should update this function comment.
*/
static boolean CheckerDT_treeCheck(Node_T oNNode) {
   size_t ulIndex;

   if(oNNode!= NULL) {

      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return FALSE;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         Node_T oNChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

         if(iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }

         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
         if(!CheckerDT_treeCheck(oNChild))
            return FALSE;
      }
   }
   return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized)
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }

   if(CheckerDT_countNodes(oNRoot) != ulCount) {
      fprintf(stderr, "Node count does not match the nodes in the tree\n");
      return FALSE;
}
   /* Now checks invariants recursively at each node from the root. */
   return CheckerDT_treeCheck(oNRoot);
}

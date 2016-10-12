/*
 * Internal bits for function handling
 */

#ifndef _CTWM_FUNCTIONS_INTERNAL_H
#define _CTWM_FUNCTIONS_INTERNAL_H


/* Keep in sync with ExecuteFunction() in external functions.h */
#define EF_FULLPROTO \
      int   func,   void *  action,   Window   w,   TwmWindow *  tmp_win, \
      XEvent *  eventp,   int   context,   bool   pulldown
#define EF_ARGS \
    /*int */func, /*void **/action, /*Window */w, /*TwmWindow **/tmp_win, \
    /*XEvent **/eventp, /*int */context, /*bool */pulldown


#endif /* _CTWM_FUNCTIONS_INTERNAL_H */
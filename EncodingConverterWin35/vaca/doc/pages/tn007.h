namespace vaca {

/**

@page page_tn_007 TN007: Calling CreateWindowEx without text

@li @ref page_tn_007_where
@li @ref page_tn_007_mdilistmenu
@li @ref page_tn_007_styles

@section page_tn_007_where Where is called CreateWindowEx?

@msdn{CreateWindowEx} is called from Widget#createHandle (a private
method). In the call is specified an empty text for the window. In
general, properties that can be changed later should be avoided to use
in the @b createHandle. It was designed in this way mainly to have @em
only the necessary arguments in the @b createHandle.

@section page_tn_007_mdilistmenu MdiListMenu problems

This has a little implication for @c MdiChild#MdiChild.  Let me
explain you. When you create a @c MdiChild, the @c MdiListMenu must be
updated (we need to add the title of that @c MdiChild in the list),
this is done automatically by Windows with the text specified in the
@c CreateWindowEx. But because Vaca doesn't specify that text in the
@c CreateWindowEx (the Frame::Frame() does the Widget::setText),
the constructor of @c MdiChild sends a WM_MDIREFRESHMENU to its
parent (an @c MdiClient) to update the @c MdiListMenu.

@see Widget#createHandle,
     MdiChild#MdiChild,
     MdiListMenu

@section page_tn_007_styles Why are styles specified in createHandle if them can be changed later?

That is not true, there are some styles that MUST be specified
in the creation of the HWND and can't be changed latter. For example:
@c ES_LEFT, @c ES_CENTER and @c ES_RIGHT.

*/

}

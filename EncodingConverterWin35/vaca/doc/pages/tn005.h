namespace vaca {

/**

@page page_tn_005 TN005: MdiChild limitations when construct it (Win32)

Win32 makes visible all MdiChild when they are created. There are no
way to fix this: Vaca uses @c CreateWindowEx with the @c WS_EX_MDICHILD
style, also I tried to use @c WM_MDICREATE without passing @c WS_VISIBLE
but has the same behaviour (I think that it's impossible to create a hidden
MdiChild).

Also, Windows ignores some other styles (for example: you can't
create a MdiChild without the WS_MAXIMIZEBOX style). But is possible
to change that styles after creation (using vaca::Widget::removeStyle or
vaca::Widget::addStyle).

Other problem is that because MdiChild is visible when it's
constructed, we lost some messages (like WM_SETFOCUS, WM_MDIACTIVATE): They
are passed directly to MdiChild#defWndProc (DefMDIChildProc finally).
So MdiChild#wndProc can't hook these events, and the onFocusEnter() and
onActivate() events aren't generated.

To fix this, we repost various messages to the message queue using
@msdn{PostMessage}:
@li @c WM_SETFOCUS to generate Widget#onFocusEnter.
@li @c WM_MDIACTIVATE to generate Frame#onActivate.
@li @c WM_SIZE to generate Frame#onResize (and layout children).

*/

}

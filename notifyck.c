#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include "config.h"

/* enums */
enum {fg, bg}; 

/* globals */
static Display *dpy = NULL;
static Window root;
static int screen;
static Colormap colormap;
static Visual *visual;
static XftColor colors[2] = {0, 0};
static int height = 100;

static void
initcolors(char *fgcol, char *bgcol)
{
	XrmValue col;
	XrmDatabase xrdb = XrmGetStringDatabase(XResourceManagerString(dpy));
	XrmGetResource(xrdb, fgcol, "String", &(char *){NULL}, &col);
	XftColorAllocName(dpy, visual, colormap, col.addr, &colors[fg]);
	XrmGetResource(xrdb, bgcol, "String", &(char *){NULL}, &col);
	XftColorAllocName(dpy, visual, colormap, col.addr, &colors[bg]);
}

Window
createwin(int x, int y, int w, int h)
{
	XSetWindowAttributes swa;
	XClassHint ch = {"notifyck", "notifyck"};

	/* setup window */
	swa.override_redirect = True;
	swa.background_pixel = colors[bg].pixel;
	swa.event_mask = ButtonPressMask | ExposureMask;
    Window win = XCreateWindow(dpy, root, x, y, w, h, 0,
			CopyFromParent, CopyFromParent, CopyFromParent,
			CWOverrideRedirect | CWBackPixel | CWEventMask, &swa);
	XSetClassHint(dpy, win, &ch);

	return win;
}

static Pixmap
createpix(GC gc, int w, int h)
{
	Pixmap pix = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
	XSetForeground(dpy, gc, colors[bg].pixel);
	XFillRectangle(dpy, pix, gc, 0, 0, w, h);
	XSetForeground(dpy, gc, colors[fg].pixel);

	return pix;
}

static void
listen(GC gc, Pixmap pix, bool once)
{
	XEvent ev;

	/* main event loop */
	bool exposed = false;
	while(!XNextEvent(dpy, &ev)) {
		if (exposed && once) {
			return;
		} switch(ev.type) {
			case Expose:
				XCopyArea(dpy, pix, ev.xexpose.window, gc,
						0, 0, WIDTH, height, 0, 0);
				exposed = true;
				break;
			case ButtonPress:
				return;
		}
	}
}

static void
usage()
{
	fputs("usage: notifyck [-u] [-t title] text\n"\
			"-u\turgent, no disappearing, urgent coloring\n"\
			"-t\toptinial title/header\n", stderr);
}

static int
draw(Drawable pix, XftFont *font, char *title, char *text)
{
	int x, y;
	size_t len = strlen(text);
	XGlyphInfo ext;
	XftDraw *draw = XftDrawCreate(dpy, pix, visual, colormap);

	/* get text extents */
	XftTextExtentsUtf8(dpy, font, text, len, &ext);
	x = ext.height/2;
	y = ext.height;
	height = ext.height*1.5;

	/* title is just an extra line */
	if (title) {
		height += ext.height;
		XftDrawStringUtf8(draw, &colors[fg], font, x, y, title, strlen(title));
		y += ext.height;
	}

	/* draw text */
	/* either single line if it fits or do inefficient char based reflow */
	if (ext.width < WIDTH) {
		XftDrawStringUtf8(draw, &colors[fg], font, x, y, text, len);
	} else {
		char *w = NULL;
		char l[len];
		int i = 0;
		for (; text[i]; ++i) {
			strncpy(l, text, i);
			l[i+1] = '\0';
			XftTextExtentsUtf8(dpy, font, l, i, &ext);

			/* when going out of bounds draw */
			if (ext.width > WIDTH-ext.height) {
				XftDrawStringUtf8(draw, &colors[fg], font, x, y, l, i);
				y += ext.height;
				height += ext.height;
				text += i;
				i = 0;
			}
		}
		XftDrawStringUtf8(draw, &colors[fg], font, x, y, l, i-1);
	}

	XftDrawDestroy(draw);
}

int
main(int argc, char *argv[])
{
	bool urgent = false;
	char *title = NULL;

	/* handle options, update values */
	unsigned c;
	while ((c = getopt(argc, argv, "ut:h")) != -1) {
		switch (c) {
			case 'u':
				urgent = true;
				break;
			case 't':
				title = malloc(strlen(optarg)+1);
				strcpy(title, optarg);
				break;
			case 'h':
			default:
				usage();
				return 1;
		}
	}

	/* non arguments are assumed to be text for notification */
	char text[TEXTMAX] = {'\0'};
	for (int i = optind; i < argc &&
			strlen(text)+strlen(argv[i])+1 < TEXTMAX; ++i) {
		strcat(text, argv[i]);
		strcat(text, " ");
	}

	/* init resources */
	dpy = XOpenDisplay(0);
	if (!dpy) {
		fprintf(stderr, "error opening display");
		return 1;
	}
	root = XDefaultRootWindow(dpy);
	screen = DefaultScreen(dpy);
	XftFont *font = XftFontOpenName(dpy, screen, FONT);
	if (!font) {
		fprintf(stderr, "error opening font '%s'", FONT);
		XCloseDisplay(dpy);
		return 1;
	}
	colormap = DefaultColormap(dpy, screen);
	visual = DefaultVisual(dpy, screen);
	GC gc = XCreateGC(dpy, root, 0, NULL);
	initcolors(urgent ? URGFG : NORFG, urgent ? URGBG : NORBG);
	Pixmap pix = createpix(gc, WIDTH, 1080);
	draw(pix, font, title, text);

	/* prepare and spawn window */
	Window win = createwin(POSX, POSY, WIDTH, height);
	XMapWindow(dpy, win);
	XFlush(dpy);

	/* all setup done, fork off */
	if (daemon(1, 1) < 0) {
		err(1, "daemon");
	}

	/* if urgent indefinitely wait for click,
	 * otherwise die after delay */
	if (urgent) {
		listen(gc, pix, false);
	} else {
		listen(gc, pix, true);
		sleep(DELAY);
	}

	/* unmap the window */
	XUnmapWindow(dpy, win);

	/* clear resources */
	XDestroyWindow(dpy, win);
	XFreePixmap(dpy, pix);
	XFreeGC(dpy, gc);
	XftColorFree(dpy, visual, colormap, &colors[fg]);
	XftColorFree(dpy, visual, colormap, &colors[bg]);
	XftFontClose(dpy, font);
	XCloseDisplay(dpy);
	return 0;
}

/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef GD_ACTIVITY_H
#define GD_ACTIVITY_H

#include "config.h"

class App;

/**
 * @ingroup Framework
 * @brief An activity object is a mini-application controlled by the user in the
 * game - for example the title screen, a file selection dialog or the game
 * playing.
 * 
 * The activity objects are managed by an App object, which organizes them in
 * a stack. Every user event (keypress etc.) is always sent to the topmost activity,
 * which is an instance of a derived class of this Activity class.
 * 
 * Activities are free to do just like anything to the screen. They can measure time
 * by overloading the Activity::timer_event() function; they can receive keypresses
 * by overloading the Activity::keypress_event() function. They are notified when
 * they become the topmost one by calling their Activity::shown_event() method,
 * and when occluded by a new activity, by their Activity::hidden_event() method.
 * Their redraw_event() is called by the App object, when they must redraw the screen.
 * 
 * Activities can enqueue Command objects in the queue of the App. After calling any
 * of the event methods, the App will check if there are any commands enqueued. If so,
 * they are executed, and this way the activities can control the flow of the whole
 * application.
 */
class Activity {
public:
    /** The keycode given to an activity is either one of the
     * App::KeyCodeSpecialKey codes, or a Unicode encoded character. */
    typedef unsigned KeyCode;
    /** Construct an activity.
     * @param app The App, in which the activity will run. */
    Activity(App *app) : app(app) {}
    /** Virtual destructor.
     * Pure virtual, but implemented in the cpp file: a trick to make
     * this an abstract base class. */
    virtual ~Activity() = 0;
    
    /**
     * Inform the Activity object about time elapsing.
     * @param ms_elapsed The number of milliseconds elapsed. These are
     * usually not real milliseconds, but the interval of the timer used
     * by the App. */
    virtual void timer_event(int ms_elapsed);
    /**
     * This is another timer, which is seldom used. It is called only in
     * replay video saving mode, and implemented only by the
     * ReplaySaverActivity class. The timer2 events are generated there
     * by the SDL_Mixer library, to use the audio mixing routine for the
     * cave replay to be totally in sync with the framerate of the game. */
    virtual void timer2_event();
    /**
     * A key pressed, sent to the Activity.
     * @param keycode a unicode character code, or some special key (see KeyCodeSpecialKey)
     * @param gfxlib_keycode graphics library specific keycode (sdl/gtk/etc). This is
     * here so the Activity can know it and also catch it, to be able to configure the
     * GameInputHandler object.
     */
    virtual void keypress_event(KeyCode keycode, int gfxlib_keycode);
    /**
     * Called by the App to request the topmost Activity to redraw the screen.
     * The state of the Activity should not change, as it may be called any time.
     * When the Activity becomes the topmost one, this is called by the App
     * automatically. Activities can call their own redraw_event() methods if
     * they want.
     */
    virtual void redraw_event();
    /**
     * Called by the App when the Activity gets pushed into the stack of activities
     * of the App. The Activity can perform some post-initialization, it may even
     * pop itself. The Command
     * objects enqueued in this event will be executed when the Activity is already
     * in the stack, so new activities they may create will be above it in the stack.
     * An activity will only receive one pushed_event() in its lifetime. */
    virtual void pushed_event();
    /**
     * Called by the App when the activities occluding the current activities have
     * disappeared. May perform any actions, but it is not necessary to do a redraw,
     * because the redraw_event will also be called. An activity may receive many
     * shown_event()-s in its lifetime. */
    virtual void shown_event();
    /**
     * Called by the App when the Activity is occluded by a new one. May free some
     * resources, for example, to be reacquired later, when the Acitivity becomes
     * topmost again. An activity may be sent many shown_event()-s in its lifetime. */
    virtual void hidden_event();

protected:
    /** The owner app. Used for enqueueing Command objects, for example. */
    App *app;

private:
    Activity(Activity const &);       ///< Not meant to be copied.
    void operator=(Activity const &); ///< Not meant to be assigned.
    
};

#endif // GD_ACTIVITY_H

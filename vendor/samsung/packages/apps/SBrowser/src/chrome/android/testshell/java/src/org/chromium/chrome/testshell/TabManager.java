// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.testshell;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;

import org.chromium.content.browser.ContentVideoViewClient;
import org.chromium.content.browser.ContentViewClient;
import org.chromium.content.browser.ContentViewRenderView;
import org.chromium.ui.base.WindowAndroid;

/**
 * The TabManager hooks together all of the related {@link View}s that are used to represent
 * a {@link TestShellTab}.  It properly builds a {@link TestShellTab} and makes sure that the
 * {@link TestShellToolbar} and {@link ContentViewRenderView} show the proper content.
 */
public class TabManager extends LinearLayout {
    private static final String DEFAULT_URL = "http://www.google.com";

    private WindowAndroid mWindow;
    private ContentVideoViewClient mContentVideoViewClient;
    private ViewGroup mContentViewHolder;
    private ContentViewRenderView mContentViewRenderView;
    private TestShellToolbar mToolbar;

    private TestShellTab mCurrentTab;

    private String mStartupUrl = DEFAULT_URL;

    /**
     * @param context The Context the view is running in.
     * @param attrs   The attributes of the XML tag that is inflating the view.
     */
    public TabManager(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
    }

    /**
     * Initialize the components required for Tab creation.
     * @param window The window used to generate all ContentViews.
     * @param videoViewClient The client to handle interactions from ContentVideoViews.
     */
    public void initialize(WindowAndroid window, ContentVideoViewClient videoViewClient) {
        assert window != null;
        mWindow = window;
        assert videoViewClient != null;
        mContentVideoViewClient = videoViewClient;
        mContentViewHolder = (ViewGroup) findViewById(R.id.content_container);
        mToolbar = (TestShellToolbar) findViewById(R.id.toolbar);
        mContentViewRenderView = new ContentViewRenderView(getContext(), mWindow) {
            @Override
            protected void onReadyToRender() {
                if (mCurrentTab == null) createTab(mStartupUrl);
            }
        };
        mContentViewHolder.addView(mContentViewRenderView,
                new FrameLayout.LayoutParams(
                        FrameLayout.LayoutParams.MATCH_PARENT,
                        FrameLayout.LayoutParams.MATCH_PARENT));
    }

    /**
     * @param startupUrl The URL that the first tab should navigate to.
     */
    public void setStartupUrl(String startupUrl) {
        mStartupUrl = startupUrl;
    }

    /**
     * @return The currently visible {@link TestShellTab}.
     */
    public TestShellTab getCurrentTab() {
        return mCurrentTab;
    }

    /**
     * Creates a {@link TestShellTab} with a URL specified by {@code url}.
     * @param url The URL the new {@link TestShellTab} should start with.
     */
    public void createTab(String url) {
        if (!isContentViewRenderViewInitialized()) return;

        ContentViewClient client = new ContentViewClient() {
            @Override
            public ContentVideoViewClient getContentVideoViewClient() {
                return mContentVideoViewClient;
            }
        };
        TestShellTab tab = new TestShellTab(getContext(), url, mWindow, client);
        setCurrentTab(tab);
    }

    private boolean isContentViewRenderViewInitialized() {
        return mContentViewRenderView != null && mContentViewRenderView.isInitialized();
    }

    private void setCurrentTab(TestShellTab tab) {
        if (mCurrentTab != null) {
            mContentViewHolder.removeView(mCurrentTab.getContentView());
            mCurrentTab.destroy();
        }

        mCurrentTab = tab;

        mToolbar.showTab(mCurrentTab);
        mContentViewHolder.addView(mCurrentTab.getContentView());
        mContentViewRenderView.setCurrentContentView(mCurrentTab.getContentView());
        mCurrentTab.getContentView().requestFocus();
    }
}
package tv.danmaku.local;

import android.media.MediaPlayer;
import android.view.SurfaceHolder;
import android.util.Log;

public class Player
{
    private MediaPlayer mMediaPlayer = null;

    private class OnErrorListener implements MediaPlayer.OnErrorListener
    {
        @Override
        public boolean onError(final MediaPlayer mp, final int what, final int extra)
        {
            Player.this.onError(what, extra);
            return true;
        }
    }

    private class OnInfoListener implements MediaPlayer.OnInfoListener
    {
        @Override
        public boolean onInfo(final MediaPlayer mp, final int what, final int extra)
        {
            Player.this.onInfo(what, extra);
            return true;
        }
    }

    private class OnVideoSizeListener implements MediaPlayer.OnVideoSizeChangedListener
    {
        @Override
        public void onVideoSizeChanged(final MediaPlayer mp, final int width, final int height)
        {
            Player.this.onVideoSize(width, height);
        }
    }

    private class OnPreparedListener implements MediaPlayer.OnPreparedListener
    {
        @Override
        public void onPrepared(final MediaPlayer mp)
        {
            Player.this.onPrepared();
        }
    }

    private class OnCompleteListener implements MediaPlayer.OnCompletionListener
    {
        @Override
        public void onCompletion(final MediaPlayer mp)
        {
            Player.this.onComplete();
        }
    }

    public Player()
    {
        mMediaPlayer = new MediaPlayer();
        mMediaPlayer.setOnErrorListener(new OnErrorListener());
        mMediaPlayer.setOnInfoListener (new OnInfoListener ());
        mMediaPlayer.setOnVideoSizeChangedListener(new OnVideoSizeListener());
        mMediaPlayer.setOnPreparedListener(new OnPreparedListener());
        mMediaPlayer.setOnCompletionListener(new OnCompleteListener());
    }

    public void setDataSource(String path)
    {
        try
        {
            mMediaPlayer.setDataSource(path);
        }
        catch(IllegalStateException e)
        {
            Log.e("JPlayer", e.getMessage());
        }
        catch(java.io.IOException e)
        {
            onError(0, 0);
        }
        catch(IllegalArgumentException e)
        {
            onError(0, 0);
        }
        catch(SecurityException e)
        {
            onError(0, 0);
        }
    }

    public void prepare()
    {
        try
        {
            mMediaPlayer.prepare();
        }
        catch(java.io.IOException e)
        {
            onError(0, 0);
        }
    }

    public void prepareAsync()
    {
        mMediaPlayer.prepareAsync();
    }

    public void start()
    {
        mMediaPlayer.start();
    }

    public void stop()
    {
        mMediaPlayer.stop();
    }

    public void pause()
    {
        mMediaPlayer.pause();
    }

    public void release()
    {
        mMediaPlayer.release();
    }

    public void seekTo(int msec)
    {
        mMediaPlayer.seekTo(msec);
    }

    public int getCurrentPosition()
    {
        return mMediaPlayer.getCurrentPosition();
    }

    public int getDuration()
    {
        return mMediaPlayer.getDuration();
    }

    public void setLooping(boolean looping)
    {
        mMediaPlayer.setLooping(looping);
    }

    public boolean isLooping()
    {
        return mMediaPlayer.isLooping();
    }

    public void setDisplay(SurfaceHolder sh)
    {
        mMediaPlayer.setDisplay(sh);
    }

    native private void onError(final int what, final int extra);
    native private void onInfo (final int what, final int extra);
    native private void onVideoSize(final int width, final int height);
    native private void onPrepared();
    native private void onComplete();
}

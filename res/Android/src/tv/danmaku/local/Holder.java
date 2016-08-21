package tv.danmaku.local;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;

public class Holder implements SurfaceHolder
{
    private Surface mSurface;

    public Holder(SurfaceTexture texture)
    {
        mSurface = new Surface(texture);
    }

    @Override
    public void addCallback(SurfaceHolder.Callback callback)
    {
    }

    @Override
    public Surface getSurface()
    {
        return mSurface;
    }

    @Override
    public Rect getSurfaceFrame()
    {
        return new Rect();
    }

    @Override
    public boolean isCreating()
    {
        return false;
    }

    @Override
    public Canvas lockCanvas(Rect dirty)
    {
        return new Canvas();
    }

    @Override
    public Canvas lockCanvas()
    {
        return new Canvas();
    }

    @Override
    public void removeCallback(SurfaceHolder.Callback callback)
    {
    }

    @Override
    public void setFixedSize(int width, int height)
    {
    }

    @Override
    public void setFormat(int format)
    {
    }

    @Override
    public void setKeepScreenOn(boolean screenOn)
    {
    }

    @Override
    public void setSizeFromLayout()
    {
    }

    @Override
    public void setType(int type)
    {
    }

    @Override
    public void unlockCanvasAndPost(Canvas canvas)
    {
    }
}

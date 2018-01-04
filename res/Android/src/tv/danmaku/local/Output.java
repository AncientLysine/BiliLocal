package tv.danmaku.local;

import android.graphics.SurfaceTexture;

public class Output extends SurfaceTexture
{
    private class OnFrameAvailableListener implements SurfaceTexture.OnFrameAvailableListener
    {
        @Override
        public void onFrameAvailable(final SurfaceTexture st)
        {
            Output.this.onAvailable();
        }
    }

    public Output()
    {
        super(0);

        setOnFrameAvailableListener(new OnFrameAvailableListener());
    }

    native private void onAvailable();
}

using System;
using System.Runtime.InteropServices;

public static class NativeOggEncoder
{
#if UNITY_IOS && !UNITY_EDITOR
    private const string LibName = "__Internal";
#else
    private const string LibName = "native_ogg_encoder";
#endif

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int noe_encode_planar(
        IntPtr[] channelPtrs,
        int numFrames,
        int channels,
        int inputSampleRate,
        int outputSampleRate,
        float quality,
        out IntPtr outData,
        out int outSize
    );

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void noe_free(IntPtr data);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr noe_get_error();

    public static byte[] ConvertToBytes(float[][] samples, int sampleRate, int channels, float quality)
    {
        return ConvertToBytes(samples, sampleRate, sampleRate, channels, quality);
    }

    public static byte[] ConvertToBytes(float[][] samples, int inputSampleRate, int outputSampleRate, int channels, float quality)
    {
        if (samples == null || samples.Length == 0)
            throw new ArgumentException("samples cannot be null or empty");

        int numFrames = samples[0].Length;
        int actualChannels = Math.Min(channels, samples.Length);

        var handles = new GCHandle[actualChannels];
        var ptrs = new IntPtr[actualChannels];

        try
        {
            for (int ch = 0; ch < actualChannels; ch++)
            {
                handles[ch] = GCHandle.Alloc(samples[ch], GCHandleType.Pinned);
                ptrs[ch] = handles[ch].AddrOfPinnedObject();
            }

            int result = noe_encode_planar(
                ptrs, numFrames, actualChannels,
                inputSampleRate, outputSampleRate,
                quality, out IntPtr outData, out int outSize
            );

            if (result != 0)
            {
                IntPtr errPtr = noe_get_error();
                string err = errPtr != IntPtr.Zero ? Marshal.PtrToStringAnsi(errPtr) : "Unknown error";
                throw new Exception($"NativeOggEncoder failed ({result}): {err}");
            }

            var output = new byte[outSize];
            Marshal.Copy(outData, output, 0, outSize);
            noe_free(outData);
            return output;
        }
        finally
        {
            for (int ch = 0; ch < actualChannels; ch++)
            {
                if (handles[ch].IsAllocated)
                    handles[ch].Free();
            }
        }
    }
}

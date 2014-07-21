VER=(0.9.4 0.10.14 0.11.5 1.0.9 1.1.12 1.2.7 2.0.5 2.1.5 2.2.5)
time {
    for V in ${VER[@]}; do
      FFDIR=ffmpeg-${V}-linux-x86+x64
      echo $FFDIR
      rm -rf $FFDIR
      mkdir -p $FFDIR/{bin,lib}/x64 
      cp -af ffmpeg-${V}/sdk-x86/* $FFDIR
      cp -af ffmpeg-${V}/sdk/bin/* $FFDIR/bin/x64
      cp -af ffmpeg-${V}/sdk/lib/* $FFDIR/lib/x64
      find $FFDIR/lib -type f -exec md5sum {} \; >$FFDIR/md5    
      tar -I pxz -cf ${FFDIR}.tar.xz $FFDIR
      du -h ${FFDIR}.tar.xz
    done
}

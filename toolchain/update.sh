#! /bin/bash
sudo umount context/*
sudo rm -rf context
mkdir context
cd context

create_tar_gz()
{
  ARCH=$1
  API=$2
  REVISION=$3
  
  NAME=$ARCH-$API
  ZIP=$NAME\_$REVISION.zip
  OUTPUT=$NAME.tar.gz

  wget https://dl.google.com/android/repository/sys-img/android/$ZIP
  unzip $ZIP $ARCH/system.img
  
  mkdir $NAME
  sudo mount $ARCH/system.img $NAME
  sudo tar cpf $OUTPUT --transform "s,^$NAME,$NAME/system," $NAME
  sudo umount $NAME
  sudo rm -rf $ZIP $NAME $ARCH
}
create_tar_gz armeabi-v7a 21 r04
create_tar_gz arm64-v8a 24 r07
create_tar_gz x86 24 r08
create_tar_gz x86_64 24 r08

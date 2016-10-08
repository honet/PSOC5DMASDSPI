# PSOC5DMASDSPI
SPI mode SD-Card interface for PSoC5LP with DMA. (fatfs porting) 

Chan氏のfatfsライブラリをPSoC5LP向けに実装したComponentLibraryです。

![]doc/DMASDSPITest.png

## fatfs
[fatfs](http://elm-chan.org/fsw/ff/00index_j.html)

DMASDSPIに同梱されているライブラリアーカイブ(/DMASDSPITest.cydsn/DMASDSPI/Library/libfatfs[d].a) は
libfatfsプロジェクトによってビルドしたものです。
特許に配慮しLFN, exFAT, は無効にしてビルドされていますので、
必要に応じて設定を変えてリビルドしてください。
リビルド後、copyarlib.bat を実行するとlibfatfsのビルド結果から
DMASDSPIコンポーネントのLibraryフォルダにアーカイブライブラリがにコピーされます。

また、設定を変えた場合は /libfatfs.cylib/ffconf.h に
/DMASDSPITest.cydsn/DMASDSPI/API/fatfs.h 内の設定defineを同じ値にして下さい。


## ベンチマーク

いくつかのSDカードでのテスト結果です。CY8CKIT-059 に実装し、
システムバスクロック48MHz, SDカードSCLK 12MHz設定したものです。

![]doc/DMASDSPISpeedTest.png


## その他注意
- TopDesignに２つ以上配置された場合の挙動は未確認です。


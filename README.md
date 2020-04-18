HYPERサイコロ将棋AIの基本ライブラリです。
UNIX系向けですので，Visual Studio等ではコンパイルできません。

makefileを同梱しています。
>make

の実行でビルドが可能です。



# 構成について

1. コア

    盤面などのゲーム状態を管理する機構を実装し、ゲーム状態の取得や指し手の実行を行うためのインタフェースを提供する。

    - board.c

        盤面や手番などのゲーム情報を保持し、それらの取得と操作を実装する。
また、ゲーム情報を元に計算される指し手の生成も行う。

    - board.h

        ゲーム状態に関わる以下を提供する。
        - ゲーム状態の取得や操作を行うためのインタフェース(マクロと関数)
        - ゲームの状態を表現するための構造体
        - 駒の種類やマスクを示す定数
        - 指し手から情報を取得するマクロ


2. アプリケーション

    boardによって管理されるゲーム状態を用いて、AIとの対戦を実現する。

    - header.h

        アプリケーション層について以下の記述を行う。
        - プログラムの情報や設定
        - アプリケーション層の関数プロトタイプ宣言

    - data.c

        盤面を表示するための文字列データ

    - ini.c

        ゲームの初期化と終了を行う

    - io.c

        入出力に関わる以下の処理を実現する
        - シェルやログファイルへの出力
        - ゲーム状態を出力するための、ビット列からの形式変換
        - シェルからのコマンド入力の受け付けと、対応する処理の実行

    - main.c

        ゲームの初期化、ゲーム中のコマンド受け取りループ、終了時処理を実現する。

    - search.c

        探索と盤面の静的評価を行う


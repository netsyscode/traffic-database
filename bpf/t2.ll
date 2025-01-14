; ModuleID = 't2.c'
source_filename = "t2.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nounwind uwtable
define dso_local i64 @entry(i8* nocapture %0) local_unnamed_addr #0 {
  %2 = bitcast i8* %0 to i8**
  %3 = load i8*, i8** %2, align 64, !tbaa !2
  %4 = getelementptr inbounds i8, i8* %0, i64 16
  %5 = bitcast i8* %4 to i16*
  %6 = load i16, i16* %5, align 16, !tbaa !10
  %7 = zext i16 %6 to i64
  %8 = getelementptr inbounds i8, i8* %3, i64 %7
  %9 = getelementptr inbounds i8, i8* %8, i64 12
  %10 = bitcast i8* %9 to i16*
  %11 = load i16, i16* %10, align 2, !tbaa !11
  %12 = icmp eq i16 %11, 8
  br i1 %12, label %13, label %21

13:                                               ; preds = %1
  %14 = getelementptr inbounds i8, i8* %8, i64 14
  %15 = load i8, i8* %14, align 1, !tbaa !12
  %16 = and i8 %15, -16
  %17 = icmp eq i8 %16, 64
  br i1 %17, label %18, label %21

18:                                               ; preds = %13
  %19 = getelementptr inbounds i8, i8* %0, i64 92
  %20 = bitcast i8* %19 to i32*
  store i32 5638145, i32* %20, align 4, !tbaa !13
  br label %21

21:                                               ; preds = %18, %13, %1
  ret i64 1
}

attributes #0 = { nofree norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1 "}
!2 = !{!3, !6, i64 0}
!3 = !{!"rte_mbuf", !4, i64 0, !6, i64 0, !7, i64 8, !4, i64 16, !8, i64 16, !8, i64 18, !8, i64 20, !8, i64 22, !7, i64 24, !4, i64 32, !4, i64 32, !9, i64 36, !8, i64 40, !8, i64 42, !4, i64 44, !8, i64 52, !8, i64 54, !6, i64 56, !4, i64 64, !6, i64 64, !4, i64 72, !6, i64 80, !8, i64 88, !8, i64 90, !4, i64 92}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!"any pointer", !4, i64 0}
!7 = !{!"long", !4, i64 0}
!8 = !{!"short", !4, i64 0}
!9 = !{!"int", !4, i64 0}
!10 = !{!3, !8, i64 16}
!11 = !{!8, !8, i64 0}
!12 = !{!4, !4, i64 0}
!13 = !{!9, !9, i64 0}

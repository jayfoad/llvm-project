; RUN: llc -march=amdgcn -mcpu=gfx900 < %s | FileCheck %s

declare i64 @llvm.amdgcn.ballot(i1)

; Test ballot(0)

; CHECK-LABEL: test0:
; CHECK-DAG: v_mov_b32_e32 v0, 0
; CHECK-DAG: v_mov_b32_e32 v1, 0
define i64 @test0() {
  %ballot = call i64 @llvm.amdgcn.ballot(i1 0)
  ret i64 %ballot
}

; Test ballot(1)

; CHECK-LABEL: test1:
; CHECK-DAG: v_mov_b32_e32 v0, exec_lo
; CHECK-DAG: v_mov_b32_e32 v1, exec_hi
define i64 @test1() {
  %ballot = call i64 @llvm.amdgcn.ballot(i1 1)
  ret i64 %ballot
}

; Test ballot of a comparison

; CHECK-LABEL: test2:
; CHECK: v_cmp_ne_u32_e64 s{{\[}}[[LO:[0-9]+]]:[[HI:[0-9]+]]{{\]}}, v{{[0-9]+}}, v{{[0-9]+}}
; CHECK-DAG: v_mov_b32_e32 v0, s[[LO]]
; CHECK-DAG: v_mov_b32_e32 v1, s[[HI]]
define i64 @test2(i32 %x, i32 %y) {
  %cmp = icmp eq i32 %x, %y
  %ballot = call i64 @llvm.amdgcn.ballot(i1 %cmp)
  ret i64 %ballot
}

; Test ballot of a non-comparison operation

; CHECK-LABEL: test3:
define i64 @test3(i32 %x) {
  %trunc = trunc i32 %x to i1
  %ballot = call i64 @llvm.amdgcn.ballot(i1 %trunc)
  ret i64 %ballot
}

# ServerFrameWork

게임 서버 개발 실무 경험을 바탕으로 설계한 C++ 기반 MMO 서버 프레임워크입니다.  
IOCP 기반 비동기 네트워크 처리, 멀티스레드 아키텍처, 유저 세션 관리, 샤드 및 월드 시스템 등 대규모 온라인 게임 운영에 필요한 핵심 요소를 구조화하였습니다.

본 프로젝트는 단순한 기능 구현에 그치지 않고, **개발 생산성과 안정성, 운영 효율을 모두 고려한 구조 설계**를 목표로 하였습니다.

---

![ThreadTimer Banner](https://img.shields.io/badge/C%2B%2B-High%20Performance-blue.svg) ![License: MIT](https://img.shields.io/badge/License-MIT-green.svg) ![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)

---

## 🎯 목적 및 방향성

- **실전 운영 경험에 기반한 구조 설계**
- **팀원들이 빠르게 개발에 착수할 수 있는 공통 프레임 제공**
- **반복 작업 최소화 및 디버깅/운영 편의성 강화**

---

## 🏗️ 주요 아키텍처

| 모듈 | 설명 |
|------|------|
| `Gateway` | 클라이언트 최초 접속 처리 및 인증 관리 |
| `Login` | 계정 인증, 로그인 큐 관리 |
| `Shard` | 지역/채널 단위 분산 처리. 세션 라우팅 포함 |
| `World` | 유저 상태 관리 및 월드 이벤트 처리 |
| `Character` | 캐릭터 로딩 및 저장, 상태 갱신 |
| `Supervisor` | 전체 서버 상태 모니터링 및 제어 인터페이스 제공 |

---

## 🔍 주요 모듈 설명

### 📌 `CServerSuperVisor`
- 서버 전체의 상태를 모니터링하고 제어하는 관리자 역할
- 개별 서버 프로세스들을 감시하고, 상태 변화에 따른 조치를 취함

### 📌 `CLoginServer`
- 유저의 로그인 요청을 처리하고, 계정 인증을 수행
- 로그인 성공 시 접속 가능한 채널 목록을 제공
- `AccountDBAgent`와 연동하여 DB 기반 인증 절차 수행

### 📌 `CGameServer`
- 실질적인 게임 로직을 처리하는 핵심 서버
- 캐릭터 이동, 전투, 채팅, 상태 동기화 등 주요 기능 담당
- 내부적으로 `CUser`, `CCharacter`, `CMonster` 등 구성 요소로 분기 처리

### 📌 `CWorldServer`
- 복수의 GameServer들을 통합 관리하는 상위 단위
- 월드 이벤트, 글로벌 브로드캐스트, 필드 로딩 등을 담당

### 📌 `CChannelManager`
- 채널 단위의 분산 구조 설계
- `LoginServer`와 연동되어 유저가 선택한 채널에 입장하도록 라우팅

### 📌 `AccountDBAgent`
- DB 연동 전담 모듈로, 계정 인증 및 캐릭터 정보 조회/저장 처리
- MSSQL 기반으로 구현되며, 비동기 처리를 고려한 구조

### 📌 `CProtocolDispatcher`
- 패킷 수신 후 처리할 핸들러를 식별하여 디스패칭
- 커맨드 ID 기반 분기 처리 및 내부 핸들러 호출

### 📌 `CSessionManager`
- 유저와의 접속 세션을 추적 및 관리
- 세션 타임아웃, 중복 접속, 퀵 로그인 등 상태 기반 세션 관리 기능 내장

---

## 🧠 설계 철학 및 의도

| 핵심 항목 | 설계 의도 |
|-----------|------------|
| **모듈화** | 담당 책임 단위별로 분리해 재사용성과 확장성 확보 |
| **비동기 처리** | IOCP 기반으로 네트워크 이벤트 처리, CPU Idle 최소화 |
| **스레드풀 운용** | 핵심 로직을 별도 Task로 분리하여 Blocking 최소화 |
| **데이터 일관성** | 단일 쓰레드 처리 원칙을 유지해 동기화 비용 제거 |
| **운영 친화성** | 디버깅 및 로그 추적 구조 내장, 프로세스 상태 추적 용이 |
| **성능 튜닝 고려** | TBB Allocator, 메모리 풀, 경량화된 객체 설계 적용 가능성 내포 |

---

## 🧩 아키텍처 설계 배경 요약

본 프레임워크는 단지 기능 구현을 넘어서, 다음과 같은 실무적 요구를 해결하기 위한 목적으로 설계되었습니다.

- **불규칙한 서버 다운 문제** → 세션/메모리 관리 구조 정비
- **무계획적 기능 추가** → 모듈 기반 구조화로 개발 스코프 명확화
- **수작업 운영 병목** → 자동화 스크립트 및 상태 추적 기능 도입

---

## 👤 Author Note

이 프레임워크는 단순히 서버를 “돌리는” 것을 넘어서  
**운영 가능한 수준의 확장성과 안정성을 갖춘 설계란 무엇인가**에 대한 개인적인 고민과 실무 경험을 녹여낸 결과물입니다.

---

## 🖼️ 아키텍처 시각 자료 (Architecture Visuals)

서버 구조를 시각적으로 표현한 다이어그램입니다. 각 모듈이 어떻게 연결되고 동작하는지 한눈에 확인하실 수 있습니다.

### 🔹 서버 샤드 구조 개요
![Shard 구조](/ServerFrameWork_Shard.jpg)

### 🔹 샤드 정보 흐름도
![Shard Info](/ServerFrameWork_Shard_Info.jpg)

### 🔹 월드 서버 구조
![World 구조](/ServerFrameWork_World.jpg)

### 🔹 로그인 프로세스 흐름
![Login 구조](/ServerFrameWork_Login.jpg)

---

## ⚙️ 기술 스택

- **Language**: C++
- **Concurrency**: IOCP, Thread Pool
- **Memory Management**: Memory Pool 기반 객체 관리
- **Protocol**: 커스텀 바이너리 프로토콜 + ID 기반 메시지 핸들러
- **Tooling**: Visual Studio, WinDbg, CRT Debug Heap
- **DevOps 시도**: NPC 스크립트 자동화, 배포 스크립트, 테스트 로그 기반 진단 등

---

## 🧪 향후 확장 고려 사항

- ✅ Redis 기반 유저 세션 분산 캐시 도입
- ✅ Kafka 기반 로깅 및 통계 이벤트 스트리밍
- ✅ WebSocket 게이트웨이 + REST API 연동 구조 추가
- ✅ Lua 기반 NPC 스크립트 플러그인 시스템 내장화
- ✅ 로그 기반 이상 탐지 기능
- ✅ AI를 활용한 서버 이벤트 예측 및 자동 리커버리 실험
- ✅ ChatGPT 기반 NPC 다이얼로그 생성 PoC

---

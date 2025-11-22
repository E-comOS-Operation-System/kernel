------ MODULE Scheduler ------
(* 
 * E-comOS Microkernel Scheduler Model Verification
 * Verifies scheduling fairness, deadlock freedom, and other critical properties
 *)

EXTENDS Integers, Sequences, TLC
CONSTANTS NumProcesses, TimeSlice, ProcessSet

(* Assume process set *)
ASSUME ProcessSet == 1..NumProcesses

(* System state variables *)
VARIABLES 
    activeProcess,      (* Currently running process ID (0 means idle) *)
    readyQueue,         (* Set of ready processes *)
    timeUsed,           (* Time slices used by current process *)
    scheduleCount       (* Scheduling count for fairness verification *)

(* All state variables *)
vars == <<activeProcess, readyQueue, timeUsed, scheduleCount>>

(* Type invariant *)
TypeInvariant ==
    /\ activeProcess \in {0} \cup ProcessSet
    /\ readyQueue \subseteq ProcessSet
    /\ timeUsed \in 0..TimeSlice
    /\ scheduleCount \in 0..1000  (* Prevent infinite verification *)
    /\ readyQueue \cap {activeProcess} = {}  (* Running process not in ready queue *)

(* Initial state *)
Init ==
    /\ activeProcess = 0
    /\ readyQueue = ProcessSet
    /\ timeUsed = 0
    /\ scheduleCount = 0

(* Pick new process from ready queue *)
PickNewProcess ==
    /\ activeProcess = 0
    /\ readyQueue # {}
    /\ \E p \in readyQueue:
        activeProcess' = p
        /\ readyQueue' = readyQueue \ {p}
        /\ timeUsed' = 0
        /\ scheduleCount' = scheduleCount + 1
    /\ UNCHANGED <<>>

(* Continue running current process *)
ContinueCurrent ==
    /\ activeProcess # 0
    /\ timeUsed < TimeSlice
    /\ activeProcess' = activeProcess
    /\ readyQueue' = readyQueue
    /\ timeUsed' = timeUsed + 1
    /\ UNCHANGED scheduleCount

(* Time slice expired, reschedule *)
TimeSliceExpired ==
    /\ activeProcess # 0
    /\ timeUsed = TimeSlice
    /\ activeProcess' = 0
    /\ readyQueue' = readyQueue \cup {activeProcess}
    /\ timeUsed' = 0
    /\ UNCHANGED scheduleCount

(* Next state relation *)
Next ==
    \/ PickNewProcess
    \/ ContinueCurrent  
    \/ TimeSliceExpired

(* Complete specification *)
Spec == Init /\ [][Next]_vars

(* ========== Properties to Verify ========== *)

(* 1. Type safety *)
Invariant == TypeInvariant

(* 2. No deadlock: always a process can run or be scheduled *)
Liveness ==
    \/ activeProcess # 0
    \/ readyQueue # {}

(* 3. Fairness: every process will eventually be scheduled *)
Fairness ==
    \A p \in ProcessSet: 
        <> (activeProcess = p)

(* 4. Progress guarantee: won't stay forever in the same process *)
Progress ==
    ~[] (activeProcess # 0 /\ timeUsed < TimeSlice)

(* Theorems to verify *)
THEOREM Spec => []Invariant
THEOREM Spec => []Liveness  
THEOREM Spec => Fairness

====

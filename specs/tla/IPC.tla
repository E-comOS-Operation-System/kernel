------ MODULE IPC ------
(*
 * E-comOS Microkernel IPC Communication Model Verification
 * Verifies message passing correctness, no message loss, and security properties
 *)

EXTENDS Integers, Sequences, TLC
CONSTANTS NumProcesses, MessageSet, MaxQueueSize

(* Process and message sets *)
ASSUME ProcessSet == 1..NumProcesses

VARIABLES
    messageQueues,     (* Message queues per process: [process -> Seq(Message)] *)
    capabilities,      (* Capability matrix: [sender -> [receiver -> BOOLEAN]] *)
    sentMessages,      (* Set of sent messages (for tracking) *)
    deliveredMessages  (* Set of delivered messages *)

(* Type definition *)
Message == [sender: ProcessSet, receiver: ProcessSet, content: MessageSet]

(* Type invariant *)
TypeInvariant ==
    /\ messageQueues \in [ProcessSet -> Seq(Message)]
    /\ capabilities \in [ProcessSet -> [ProcessSet -> BOOLEAN]]
    /\ sentMessages \subseteq SUBSET Message
    /\ deliveredMessages \subseteq sentMessages
    /\ \A p \in ProcessSet: 
        Len(messageQueues[p]) <= MaxQueueSize

(* Initial state *)
Init ==
    /\ messageQueues = [p \in ProcessSet |-> <<>>]
    /\ capabilities = [s \in ProcessSet |-> [r \in ProcessSet |-> s = r]]  (* Default: can only send to self *)
    /\ sentMessages = {}
    /\ deliveredMessages = {}

(* Send message operation *)
SendMessage(sender, receiver, msgContent) ==
    /\ sender \in ProcessSet
    /\ receiver \in ProcessSet  
    /\ msgContent \in MessageSet
    /\ capabilities[sender][receiver] = TRUE  (* Must have send permission *)
    /\ Len(messageQueues[receiver]) < MaxQueueSize  (* Queue not full *)
    
    /\ LET newMsg == [sender |-> sender, receiver |-> receiver, content |-> msgContent]
       IN messageQueues' = [messageQueues EXCEPT ]
       /\ sentMessages' = sentMessages \cup {newMsg}
       
    /\ UNCHANGED <<capabilities, deliveredMessages>>

(* Receive message operation *)
ReceiveMessage(receiver) ==
    /\ receiver \in ProcessSet
    /\ messageQueues[receiver] # <<>>
    
    /\ LET msg == Head(messageQueues[receiver])
       IN messageQueues' = [messageQueues EXCEPT ]
          /\ deliveredMessages' = deliveredMessages \cup {msg}
          
    /\ UNCHANGED <<capabilities, sentMessages>>

(* Grant capability operation *)
GrantCapability(granter, grantee, target) ==
    /\ granter \in ProcessSet
    /\ grantee \in ProcessSet  
    /\ target \in ProcessSet
    /\ capabilities[granter][target] = TRUE  (* Granter must have the capability *)
    
    /\ capabilities' = [capabilities EXCEPT 
Next ==
    \/ \E s, r \in ProcessSet, m \in MessageSet: SendMessage(s, r, m)
    \/ \E r \in ProcessSet: ReceiveMessage(r)  
    \/ \E g, r, t \in ProcessSet: GrantCapability(g, r, t)

(* Complete specification *)
Spec == Init /\ [][Next]_vars

(* ========== Verification Properties ========== *)

(* 1. No message loss: sent messages are eventually delivered *)
NoMessageLoss ==
    \A msg \in sentMessages: msg \in deliveredMessages

(* 2. Message ordering: first-in-first-out *)
MessageOrdering ==
    \A p \in ProcessSet:
        \A i, j \in 1..Len(messageQueues[p]):
            i < j => messageQueues[p][i].sender \leq messageQueues[p][j].sender

(* 3. Capability safety: cannot send without capability *)
CapabilitySafety ==
    \A msg \in sentMessages: capabilities[msg.sender][msg.receiver]

(* 4. Queue boundedness *)
QueueBounded ==
    \A p \in ProcessSet: Len(messageQueues[p]) <= MaxQueueSize

THEOREM Spec => []NoMessageLoss
THEOREM Spec => []CapabilitySafety  
THEOREM Spec => []QueueBounded

====

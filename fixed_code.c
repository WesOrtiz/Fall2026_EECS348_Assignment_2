
/*
EECS Assignment 2
Description: implements an automated priority queue for managing unread emails, processing commands directly from a text file
Collaborators: Me and ChatGPT?? (Not sure if this is what to put)
Sources: ChatGPT? (It’s ChatGPT’s code, I just gave it a tune up)
Author: Wesley Ortiz
Creation date: 9/15/2026
Revision date: 9/17/2026
Input: Give it your file name through the terminal. It might get angry otherwise.
Revisions:
Execution time:
Old code called sscanf repeatedly which caused bottlenecking. I added long date_numeric in the email definition space. This parses the date integer string only once which speeds things up.
Correctness:
The old code had a loop that assumed formatting patterns were perfect. So if an incoming command omitted commas, the pointer returned a null reference value, which would crash. I added if (!firstComma) return 0; to skip bad configurations rather than crashing.
Space complexity:
I increased the date string buffer from 11 to 12 characters to give room for error when checking a 10 character string.
Maintainability:
I removed some global multi-assignment variables and added some more visual separation just so the code is easier to look at and follow.

Side note: Sorry if the prologue comments are terrible. I missed the first assignment so I’m trying to figure this out.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EMAILS 1000
#define SUBJECT_SIZE 500

/*
 * Structure to hold email information.
 * All fields are sized appropriately with safety margins.
 */
typedef struct {
    char sender[50];
    char subject[SUBJECT_SIZE];
    char date[12];         // Increased to 12 bytes to safely hold "MM-DD-YYYY\0"
    int priority;
    long date_numeric;     // IMPROVEMENT: Cached numeric date (YYYYMMDD) to eliminate sscanf calls in sorting
} Email;

/*
 * Fixed-size MaxHeap container structure.
 */
typedef struct {
    Email heap[MAX_EMAILS];
    int size;
} MaxHeap;

/* ---------------------------------------------------------
 * Utility Functions
 * --------------------------------------------------------- */

/*
 * Maps string categories to integer priority levels.
 */
int getPriority(const char *sender)
{
    if (strcmp(sender, "Boss") == 0) return 5;
    if (strcmp(sender, "Subordinate") == 0) return 4;
    if (strcmp(sender, "Peer") == 0) return 3;
    if (strcmp(sender, "ImportantPerson") == 0) return 2;
    return 1; // Default fallback matching original specifications
}

/*
 * Converts an "MM-DD-YYYY" string into a numeric YYYYMMDD long integer.
 * Validates input formatting to avoid processing garbage values.
 */
long parseDateToNumeric(const char *date)
{
    int month = 0, day = 0, year = 0;
    // Check if sscanf successfully extracts exactly 3 integer entities
    if (sscanf(date, "%d-%d-%d", &month, &day, &year) != 3) {
        return 0; // Return zero as a safe baseline if parsing fails
    }
    return (long)year * 10000 + (long)month * 100 + (long)day;
}

/*
 * Priority evaluator: Returns 1 if email1 is higher priority than email2, 0 otherwise.
 */
int higherPriority(const Email *email1, const Email *email2)
{
    // Compare primary sender categories first
    if (email1->priority > email2->priority) return 1;
    if (email1->priority < email2->priority) return 0;

    // Tie-breaker: Compare internal pre-calculated long values (Newer dates come first)
    if (email1->date_numeric > email2->date_numeric) return 1;

    return 0;
}

/* ---------------------------------------------------------
 * MaxHeap Operations
 * --------------------------------------------------------- */

/*
 * Preps priority queue parameters at instantiation.
 */
void initializeHeap(MaxHeap *heap)
{
    heap->size = 0;
}

/*
 * Swaps all memory content between two targeted email objects.
 */
void swap(Email *a, Email *b)
{
    Email temp = *a;
    *a = *b;
    *b = temp;
}

/*
 * Moves a newly inserted item up the heap tree until bounds are legal.
 */
void heapifyUp(MaxHeap *heap, int index)
{
    while (index > 0)
    {
        int parent = (index - 1) / 2;

        // Swap upward if child priority beats parent priority
        if (higherPriority(&heap->heap[index], &heap->heap[parent]))
        {
            swap(&heap->heap[index], &heap->heap[parent]);
            index = parent; // Advance lookup to next parent level
        }
        else
        {
            break; // Valid heap condition verified
        }
    }
}

/*
 * Sifts an out-of-order element down the binary tree iteratively.
 */
void heapifyDown(MaxHeap *heap, int index)
{
    while (1)
    {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int largest = index;

        // Check left child bounds and evaluate priority
        if (left < heap->size && higherPriority(&heap->heap[left], &heap->heap[largest])) {
            largest = left;
        }

        // Check right child bounds and evaluate priority
        if (right < heap->size && higherPriority(&heap->heap[right], &heap->heap[largest])) {
            largest = right;
        }

        // Perform standard swap if child violates heap pattern
        if (largest != index)
        {
            swap(&heap->heap[index], &heap->heap[largest]);
            index = largest; // Shift scope downward
        }
        else
        {
            break; // Structural integrity fully restored
        }
    }
}

/*
 * Safely writes a valid structural email record into the active queue layer.
 */
void insert(MaxHeap *heap, Email email)
{
    // Prevent buffer overflows against maximum allowed indices
    if (heap->size >= MAX_EMAILS)
    {
        fprintf(stderr, "Error: Priority queue limits exceeded (%d records max).\n", MAX_EMAILS);
        return;
    }

    // Insert instance payload at the current leaf boundary
    heap->heap[heap->size] = email;
    heapifyUp(heap, heap->size);
    heap->size++;
}

/*
 * Securely pops and drops the maximum element, maintaining heap balances.
 */
void removeMax(MaxHeap *heap)
{
    if (heap->size == 0) {
        fprintf(stderr, "Error: Queue underflow on extraction command.\n");
        return;
    }

    heap->size--;

    // Move structural leaf element into root slot and cascade down
    if (heap->size > 0)
    {
        heap->heap[0] = heap->heap[heap->size];
        heapifyDown(heap, 0);
    }
}

/* ---------------------------------------------------------
 * Input Parsing and Commands
 * --------------------------------------------------------- */

/*
 * Parses incoming raw buffer trace arrays safely.
 * Includes defensive checks to catch structural data errors instead of crashing.
 */
int parseEmail(char *line, Email *out_email)
{
    char *data = line + 6; // Move read index past the parsed "EMAIL " prefix marker
    
    // Scan sequentially for structural commas to avoid pointer fault assumptions
    char *firstComma = strchr(data, ',');
    if (!firstComma) return 0; // Malformed parsing line string checked safely

    char *secondComma = strchr(firstComma + 1, ',');
    if (!secondComma) return 0; // Guard against absent parameters

    // Break string into isolated null-terminated tokens temporarily
    *firstComma = '\0';
    *secondComma = '\0';

    char *sender_part = data;
    char *subject_part = firstComma + 1;
    char *date_part = secondComma + 1;

    // Remove any potential newline markers bound inside trailing token sections
    date_part[strcspn(date_part, "\r\n")] = '\0';

    // Perform safe, size-bounded copies to prevent memory safety issues
    strncpy(out_email->sender, sender_part, sizeof(out_email->sender) - 1);
    out_email->sender[sizeof(out_email->sender) - 1] = '\0';

    strncpy(out_email->subject, subject_part, sizeof(out_email->subject) - 1);
    out_email->subject[sizeof(out_email->subject) - 1] = '\0';

    strncpy(out_email->date, date_part, sizeof(out_email->date) - 1);
    out_email->date[sizeof(out_email->date) - 1] = '\0';

    // Calculate metadata variables instantly to limit looping operations downstream
    out_email->priority = getPriority(out_email->sender);
    out_email->date_numeric = parseDateToNumeric(out_email->date);

    return 1; // Mark successfully completed trace
}

/*
 * Reads the maximum node array point and pushes strings cleanly to stdout.
 */
void displayNext(MaxHeap *heap)
{
    if (heap->size == 0) {
        printf("No emails available to display.\n");
        return;
    }
    
    Email *top = &heap->heap[0];
    printf("Next email:\nSender: %s\nSubject: %s\nDate: %s\n", 
           top->sender, top->subject, top->date);
}

/* ---------------------------------------------------------
 * Application Entry
 * --------------------------------------------------------- */

int main(int argc, char *argv[])
{
    // Bound structure memory context safely inside static scopes
    MaxHeap emailQueue;
    char line[SUBJECT_SIZE + 100];

    initializeHeap(&emailQueue);

    // Validate if runtime initialization args targets valid paths
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <input_filename_path>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error: Target configuration trace file could not be read.\n");
        return 1;
    }

    // Sequentially consume stream iterations safely
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Remove tracking characters from input streams instantly
        line[strcspn(line, "\r\n")] = '\0';

        // Bypass empty line entries efficiently
        if (line[0] == '\0') continue;

        // Parse and validate incoming EMAIL transactions
        if (strncmp(line, "EMAIL ", 6) == 0)
        {
            Email parsingBuffer;
            if (parseEmail(line, &parsingBuffer)) {
                insert(&emailQueue, parsingBuffer);
            } else {
                fprintf(stderr, "Warning: Skipping completely malformed transaction input line context.\n");
            }
        }
        else if (strcmp(line, "NEXT") == 0)
        {
            displayNext(&emailQueue);
        }
        else if (strcmp(line, "READ") == 0)
        {
            removeMax(&emailQueue);
        }
        else if (strcmp(line, "COUNT") == 0)
        {
            printf("There are %d emails to read.\n", emailQueue.size);
        }
    }

    fclose(file); // Release tracking streams gracefully
    return 0;
}



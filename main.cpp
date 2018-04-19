#include <iostream>
#include <cstdio>
#include <cstdlib>

using namespace std;

typedef enum {
    OBJ_INT,
    OBJ_PAIR
} ObjectType;

typedef struct _Object{
    ObjectType type;
    unsigned char marked;

    struct _Object *next;

    union {
        int value;
        struct {
            struct _Object *head;
            struct _Object *tail;
        };
    };
} _Object;

#define STACK_MAX 256

typedef struct {
    _Object *stack[STACK_MAX];
    int stackSize;
    _Object *head;
    int numAllocatedObj;
    int maxGCCapacity;
} VM;

void sampleAssert(int condition, const char *message) {
    if(!condition){
        printf("%s\n",message);
        exit(1);
    }
}

VM* newVM() {
    VM *vm = new VM();
    vm->stackSize = 0;
    vm->head = NULL;
    vm->numAllocatedObj = 0;
    vm->maxGCCapacity = 8;
    return vm;
}

void push(VM *vm, _Object *value) {
    sampleAssert(vm->stackSize < STACK_MAX, "Stack Overflow, exiting");
    vm->stack[vm->stackSize++] = value;
}

_Object *pop(VM* vm){
    sampleAssert(vm->stackSize > 0, "Stack Underflow, exiting");
    return vm->stack[--vm->stackSize];
}

void mark(_Object *obj) {
    if(obj->marked) return;

    obj->marked = 1;

    if(obj->type == OBJ_PAIR) {
        mark(obj->head);
        mark(obj->tail);
    }
}

void markAll(VM *vm){
    for(int i = 0;i<vm->stackSize;i++){
        mark(vm->stack[i]);
    }
}

void sweep(VM* vm){
    _Object** obj = &vm->head;
    while (*obj) {
        if (!(*obj)->marked) {
            _Object* unreached = *obj;
            *obj = unreached->next;
            delete(unreached);
            vm->numAllocatedObj--;
        } else {
            (*obj)->marked = 0;
            obj = &(*obj)->next;
        }
    }
}

void gc(VM* vm) {
    int numAllocatedObj= vm->numAllocatedObj;

    markAll(vm);
    sweep(vm);

    vm->maxGCCapacity = vm->numAllocatedObj * 2;

    printf("Collected %d objects, %d remaining.\n", numAllocatedObj- vm->numAllocatedObj,
         vm->numAllocatedObj);
}

_Object* newObject(VM* vm, ObjectType type) {
    if (vm->numAllocatedObj == vm->maxGCCapacity){
        gc(vm);
    }

    _Object* obj = new _Object();
    obj->type = type;
    obj->next = vm->head;
    vm->head = obj;
    obj->marked = 0;
    vm->numAllocatedObj++;
    return obj;
}

void pushInt(VM* vm, int intValue) {
    _Object* obj = newObject(vm, OBJ_INT);
    obj->value = intValue;
    push(vm, obj);
}

_Object* pushPair(VM* vm) {
    _Object* obj = newObject(vm, OBJ_PAIR);
    obj->tail = pop(vm);
    obj->head = pop(vm);
    push(vm, obj);
    return obj;
}

void objectPrint(_Object* object) {
    switch (object->type) {
        case OBJ_INT:
        printf("%d", object->value);
        break;

        case OBJ_PAIR:
        printf("(");
        objectPrint(object->head);
        printf(", ");
        objectPrint(object->tail);
        printf(")");
        break;
    }
}

void freeVM(VM *vm) {
    vm->stackSize = 0;
    gc(vm);
    delete(vm);
}

void printList(VM *vm){
    if(!vm){
        return;
    }
    _Object *obj = vm->head;
    cout << "printing list" << endl;
    while(obj){
        cout << " type : " << obj->type << ", marked : " << (int)obj->marked << ", ";
        objectPrint(obj);
        cout << endl;
        obj = obj->next;
    }
    cout << endl;
}

void printStack(VM *vm){
    cout << "printing stack" << endl;
    for(int i = 0;i< vm->stackSize;i++){
        objectPrint(vm->stack[i]);
        cout << endl;
    }
    cout << endl;
}

void test1() {
    printf("Test 1: Objects on stack are preserved.\n");
    VM* vm = newVM();
    pushInt(vm, 1);
    pushInt(vm, 2);

    gc(vm);
    sampleAssert(vm->numAllocatedObj== 2, "Should have preserved objects.");
    freeVM(vm);
}

void test2() {
    printf("Test 2: Unreached objects are collected.\n");
    VM* vm = newVM();
    pushInt(vm, 1);
    pushInt(vm, 2);
    pop(vm);
    pop(vm);

    gc(vm);
    sampleAssert(vm->numAllocatedObj== 0, "Should have collected objects.");
    freeVM(vm);
}

void test3() {
    printf("Test 3: Reach nested objects.\n");
    VM* vm = newVM();
    pushInt(vm, 1);
    pushInt(vm, 2);
    pushPair(vm);
    pushInt(vm, 3);
    pushInt(vm, 4);
    pushPair(vm);
    pushPair(vm);

    gc(vm);
    sampleAssert(vm->numAllocatedObj== 7, "Should have reached objects.");
    sampleAssert(vm->stackSize == 1, "something is wrong");
    freeVM(vm);
}

void test4() {
    printf("Test 4: Handle cycles.\n");
    VM* vm = newVM();
    pushInt(vm, 1);
    pushInt(vm, 2);
    _Object* a = pushPair(vm);
    pushInt(vm, 3);
    pushInt(vm, 4);
    _Object* b = pushPair(vm);
    printList(vm);
    printStack(vm);
    cout << "stack Size : " << (int)vm->stackSize <<  ", allocated Objects : " << vm->numAllocatedObj << endl;
    /* Set up a cycle, and also make 2 and 4 unreachable and collectible. */
    a->tail = b;
    b->tail = a;
    cout << "stack Size : " << (int)vm->stackSize <<  ", allocated Objects : " << vm->numAllocatedObj << endl;
    // printList(vm);
    // printStack(vm);

    gc(vm);
    sampleAssert(vm->numAllocatedObj== 4, "Should have collected objects.");
    freeVM(vm);
}

void perfTest() {
    printf("Performance Test.\n");
    VM* vm = newVM();

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 20; j++) {
            pushInt(vm, i);
        }

        for (int k = 0; k < 20; k++) {
            pop(vm);
        }
    }
    freeVM(vm);
}

int main(int argc, const char * argv[]) {
    test1();
    test2();
    test3();
    test4();
    perfTest();
    return 0;
}
In this section a more practical example on how to develop using this
kernel is presented. In this example we are going to develop a service
able to encrypt and decrypt messages thanks to the Ascon library. If you
are not familiar with Ascon implementation please see if first. Moreover we are also going to see how to
perform communication with a redundant task.

-   Task declaration

    ``` {.objectivec language="C"}
    // import ascon api headers
    #include "api.h"
    #include "ascon.h"
    #include "crypto_aead.h"
    #include "permutations.h"
    #include "word.h"

    // define our task that will query taskAscon
    osThreadId_t taskUser;
    const osThreadAttr_t taskUserAttributes = {
      .name = "UserTask",
      .stack_size = 1000 * 4,
      .priority = (osPriority_t) osPriorityNormal,
    };

    // ascon service
    osThreadId_t taskAscon;
    const osThreadAttr_t taskAsconAttributes = {
      .name = "AsconTask",
      .stack_size = 4000 * 4,
      .priority = (osPriority_t) osPriorityNormal,
    };

    // define the task bodies
    void taskUserBody(void *argument);
    void taskAsconBody(void *argument);

    // define the commit functions
    void commitAscon();

    #define AD_LENGTH 16
    #define M_LENGTH 64
    #define EXEC_STATE_ACTIVE 1
    #define EXEC_STATE_INACTIVE 0
    #define EXEC_STATE_DONE 2
    #define EXEC_MODE_ENCRYPT 1
    #define EXEC_MODE_DECRYPT 0

    //LIM: cannot use pointers inside this struct
    typedef struct inputAscon {
      uint8_t exec_state;
      uint8_t exec_mode;
      unsigned char cypher[M_LENGTH + CRYPTO_ABYTES];
      unsigned long long cypher_len;
      unsigned char message[M_LENGTH];
      unsigned long long message_len;
      unsigned char ad[AD_LENGTH];
      unsigned long long ad_len;
      unsigned char nonce[CRYPTO_NPUBBYTES];
      unsigned char key[CRYPTO_KEYBYTES];
    } inputAscon_t;

    typedef struct outputAscon {
      uint8_t exec_state;
      uint8_t exec_mode;
      unsigned char cypher[M_LENGTH + CRYPTO_ABYTES];
      unsigned char message[M_LENGTH];
      unsigned long long cypher_len;
      unsigned long long message_len;
    } outputAscon_t;

    void commitAscon(){
      outputAscon_t * output = (outputAscon_t*) xGetOutput();
      if (output->exec_state == EXEC_STATE_DONE){
        if (output->exec_mode == EXEC_MODE_ENCRYPT){
          printf("OUTPUT Cypher: ");
          for (int i = 0; i < output->cypher_len; i++){
            printf("%02x", output->cypher[i]);
          }
          printf("\n");
        } else if (output->exec_mode == EXEC_MODE_DECRYPT){
          printf("OUTPUT Message: ");
          for (int i = 0; i < output->message_len; i++){
            printf("%02x", output->message[i]);
          }
          printf("\n");
        }
      }
    }
    ```

    In this part of the code our *\"user\"* task and the Ascon task are
    defined. Input and output of the Ascon task are defined based on the
    input needed and the output produced by the original Ascon
    implementation of function `crypto_aead_encrypt` and
    `crypto_aead_decrypt`.

-   Initialization

    ``` {.objectivec language="C"}
    taskAscon = osThreadNewRedundant(taskAsconBody, NULL, &taskAsconAttributes);  // in task.h xTaskCreateRedundant
    taskUser = osThreadNew(taskUserBody, NULL, &taskUserAttributes); 

    // setting the input of the ascon task
    inputAscon_t * inputAscon = pvPortMalloc(sizeof(inputAscon_t));
    inputAscon->exec_state = EXEC_STATE_INACTIVE;
    xSetInput(taskAscon, inputAscon, sizeof(inputAscon_t));

    // setting the output of the ascon task
    xSetOutput(taskAscon, sizeof(outputAscon_t));

    // set the commit function of the ascon task
    xSetCommitFunction(taskAscon, commitAscon, NULL);
    ```

    Here we create a default task `taskUser` and the redundant task
    `taskAscon`. Then the input of Ascon is initialized in a waiting
    state.

-   User task body

    ``` {.objectivec language="C"}
    void taskUserBody(void *argument)
    {
      // declarizing and initializing
      unsigned char message[M_LENGTH];
      unsigned char ad[AD_LENGTH];
      // some more code here
      //...

      // synch mechanism
      printf("\nAwaiting for the encryption service to be ready\n");

      // getting the task handle of taskAscon
      TaskHandle_t taskAscon = xTaskGetHandle("AsconTask");
      uint8_t ready = 0;
      
      // wait for the output of the ascon task to be set to inactive
      while (!ready){
        outputAscon_t * output = (outputAscon_t*) xGetTaskOutput(taskAscon);
        if (output->exec_state == EXEC_STATE_INACTIVE){
          ready = 1;
        }
        osDelay(50);
      }

      // preparing the input to the ascon task
      inputAscon_t * input = pvPortMalloc(sizeof(inputAscon_t));
      input->exec_state = EXEC_STATE_ACTIVE;
      // some more code here for initialization
      // ...

      // setting the input of the ascon task
      printf("Input set for encryption\n");
      xSetTaskInput(taskAscon, input);

      // function continues here by doing the same except calling for decrypt function
      // please see full version on examples/ascon_service

    }
    ```

    In this snippet of code we can appreciate sync mechanism using API
    call `xGetTaskOutput`, that conveniently contains the state of the
    Ascon service, to set at appropriate time a new input for the
    service using `xSetTaskInput`.

-   Ascon task body

    ``` {.objectivec language="C"}
    void taskAsconBody(void *argument){
      inputAscon_t * input = (inputAscon_t*) xGetInput();
      outputAscon_t * output = (outputAscon_t*) xGetOutput();
      for(;;){
        // an external task has to set the input
        if(input->exec_state == EXEC_STATE_ACTIVE){

          if(input->exec_mode == EXEC_MODE_ENCRYPT){
            
            crypto_aead_encrypt(input->cypher, &input->cypher_len, input->message, input->message_len, input->ad, input->ad_len, NULL, input->nonce, input->key);
            input->exec_state = EXEC_STATE_DONE;
            output->exec_state = input->exec_state;
            output->exec_mode = input->exec_mode;

            output->cypher_len = input->cypher_len;
            memcpy(output->cypher, input->cypher, input->cypher_len);
            
          } else if(input->exec_mode == EXEC_MODE_DECRYPT){
            
            crypto_aead_decrypt(input->message, &input->message_len, NULL, input->cypher, input->cypher_len, input->ad, input->ad_len, input->nonce, input->key);
            input->exec_state = EXEC_STATE_DONE;
            output->exec_state = input->exec_state;
            output->exec_mode = input->exec_mode;
            output->message_len = input->message_len;
            memcpy(output->message, input->message, input->message_len);
          
          }

        } else if (input->exec_state == EXEC_STATE_DONE) {
          input->exec_state = EXEC_STATE_INACTIVE;
          output->exec_state = input->exec_state;
        }
        osDelay(50);
      }
    }
    ```

    In this function it's worth noting the structure of the main
    for-loop: it's written in a `if-else` conditional structure that
    does avoid ambiguity even in case of a failure during computation of
    either decryption or encryption. In this case, if a fault occurs,
    the task can simply repeat the process thanks to the encoding in the
    `inputAscon_t` structure of the variable `exec_mode`, effectively
    restoring original operational state.
#include <getopt.h>
#include <iostream>
#include "config.h"
#include "task.h"
#include "train_utils.h"

#define CHECK_INT_ARG(name, value) \
if (value == -1) { \
  std::cout << "Error: `" << name "` required" << std::endl; \
  return 1; \
}

#define CHECK_STR_ARG(name, value) \
if (value == "") { \
  std::cout << "Error: `" << name "` required" << std::endl; \
  return 1; \
}

void printHelp(const std::string &programName) {
  std::cout << "Usage:" << std::endl;
  std::cout << programName <<" [OPTIONS]" << std::endl;
  std::cout << "\
Fine-tune a BERT model.\n\
See also `src/config.h` for compile-time options.\n\n\
Model and data selection:\n\
  -M, --model-dir           Extracted model directory\n\
                              The directory should include each model parameter in a\n\
                              `*.bin` 32-bit float binary file and the vocabulary in\n\
                              `vocab.txt`. Also see `extract_model.py`\n\
  -D, --data-dir            Base data directory\n\
                            The directory should include a `train-texts` and a \n\
                              `val-texts` file, as well as the equivalent\n\
                              `{train,val}-$task` files for each task\n\n\
Task selection:\n\
  -t  --task                Task name (see `--data-dir`)\n\
  -m  --metric              Add a metric for the specified task.\n\
                              Choose from: {accuracy,f1,matthewscc}\n\
  -l  --loss-multiplier     Multiplier for task loss\n\
                              Default: 0.1\n\n\
Training parameters:\n\
  -b, --batch-size\n\
  -e, --num-epochs\n\n\
Training options:\n\
  -n, --num-workers         Number of workers for the data loader.\n\
                              Default: 0 (single-threaded)\n\
";
}

int main(int argc, char *argv[]) {
  int c, opt = 0;
  int batchSize, numEpochs; batchSize = numEpochs = -1;
  int numWorkers = 0;
  std::string modelDir, dataDir; modelDir = dataDir = "";
  std::vector<Task> tasks;
  Task lastTask;

	static struct option options[] = {
			{"batch-size",            required_argument, NULL,  'b' },
			{"num-workers",           required_argument, NULL,  'w' },
			{"num-epochs",            required_argument, NULL,  'e' },
			{"model-dir",             required_argument, NULL,  'M' },
			{"data-dir",              required_argument, NULL,  'D' },
			{"task",                  required_argument, NULL,  't' },
			{"metric",                required_argument, NULL,  'm' },
			{"loss-multiplier",       required_argument, NULL,  'l' },
			{"help",                  no_argument,       NULL,  'h' },
			{NULL,                    0,                 NULL,   0  }
	};

	while ((c = getopt_long(argc, argv, "-:b:e:w:M:D:t:m:h", options, &opt)) != -1) {
    switch (c) {
      case 1:
        printf("Invalid argument '%s'\n", optarg); /* non-option arg */
        return 1;
     case 'b':
        batchSize = std::stoi(optarg);
        break;
      case 'e':
        numEpochs = std::stoi(optarg);
        break;
      case 'w':
        numWorkers = std::stoi(optarg);
        break;
      case 'M':
        modelDir = optarg;
        break;
      case 'D':
        dataDir = optarg;
        break;
      case 'h':
        printHelp(argv[0]);
        break;
      case 't':
        if (!lastTask.getName().empty()) {
          tasks.push_back(lastTask);
          lastTask = Task();
        }
        lastTask.init(optarg);
        break;
      case 'm':
        if (lastTask.getName().empty()) {
          printf("Task not specified, cannot add metric `%s`\n", optarg);
          return 1;
        }
        lastTask.addMetric(optarg);
        break;
      case 'l':
        if (lastTask.getName().empty()) {
          printf("Task not specified, cannot set loss multiplier `%s`", optarg);
          return 1;
        }
        lastTask.setLossMultiplier(std::stof(optarg));
        break;
      case '?':
        printf("Invalid argument -%c\n", optopt);
        return 1;
      case ':':
        printf("Missing option for %c\n", optopt);
        return 1;
      default:
        printf("?? getopt returned character code 0%o ??\n", c);
        return 1;
		 }
  }

  if (!lastTask.getName().empty()) {
    tasks.push_back(lastTask);
  }

  if (tasks.size() == 1) {
    tasks[0].setLossMultiplier(1.0f);
  }

  CHECK_INT_ARG("--batch-size", batchSize);
  CHECK_INT_ARG("--num-epochs", numEpochs);
  CHECK_STR_ARG("--model-dir", modelDir);
  CHECK_STR_ARG("--data-dir", dataDir);

  for (const auto& task : tasks) {
    std::cout << "Task: " << task.getName() << std::endl;
    std::cout << "\tLoss multiplier: " << task.getLossMultiplier() << std::endl;
    for (const auto& metric : task.getMetrics()) {
      std::cout << "\tMetric: " << metric << std::endl;
    }
  }

  Config config;
  runTraining(config, modelDir, dataDir, tasks, batchSize, numWorkers, numEpochs);

  return 0;
}
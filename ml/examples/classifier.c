/**
 *	file name:  ml/examples/classifier.c
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   Classifier for SmartPrince
 */
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <math.h>
#include <fcntl.h>
#include <dirent.h>
 #include <sys/types.h>
#include <sys/stat.h>

#include "debug.h"
#include "data.h"
#include "darknet.h"
#include "common/file.h"
#include "common/config.h"
#include "common/debug.h"
#include "devices/motor.h"
#include "devices/i2c.h"
#include "devices/networks.h"
#include "motion/video.h"

#include "classifier.h"

uint32_t FaceProcessing = 0;

float *get_regression_values(char **labels, int n)
{
    float *v = calloc(n, sizeof(float));
    int i;
    for(i = 0; i < n; ++i){
        char *p = strchr(labels[i], ' ');
        *p = 0;
        v[i] = atof(p+1);
    }
    return v;
}

void train_classifier(char *datacfg, char *cfgfile, char *weightfile, int *gpus, int ngpus, int clear)
{
    int i;

    float avg_loss = -1;
    char *base = basecfg(cfgfile);
    printf("%s\n", base);
    printf("%d\n", ngpus);
    network **nets = calloc(ngpus, sizeof(network*));

    srand(time(0));
    int seed = rand();
    for(i = 0; i < ngpus; ++i){
        srand(seed);
#ifdef GPU
        cuda_set_device(gpus[i]);
#endif
        nets[i] = load_network(cfgfile, weightfile, clear);
        nets[i]->learning_rate *= ngpus;
    }
    srand(time(0));
    network *net = nets[0];

    int imgs = net->batch * net->subdivisions * ngpus;

    printf("Learning Rate: %g, Momentum: %g, Decay: %g\n", net->learning_rate, net->momentum, net->decay);
    list *options = read_data_cfg(datacfg);

    char *backup_directory = option_find_str(options, "backup", "backup/");
    int tag = option_find_int_quiet(options, "tag", 0);
    char *label_list = option_find_str(options, "labels", "data/labels.list");
    char *train_list = option_find_str(options, "train", "data/train.list");

    char *tree = option_find_str(options, "tree", 0);
    if (tree) net->hierarchy = read_tree(tree);
    int classes = option_find_int(options, "classes", 2);

    char **labels = 0;
    if(!tag){
        labels = get_labels(label_list);
    }
    list *plist = get_paths(train_list);
    char **paths = (char **)list_to_array(plist);
    printf("plist->size:N: %d\n", plist->size);
    int N = plist->size;    ///10
    double time;

    /**
    for(i=0; i<N; i++)
        printf("paths[%d]: %s\n", i, paths[i]);
    */

    load_args args = {0};
    args.w = net->w;
    args.h = net->h;
    ///args.threads = 32;
    args.threads = 1;
    args.hierarchy = net->hierarchy;

    args.min = net->min_ratio*net->w;
    args.max = net->max_ratio*net->w;
    printf("%d %d\n", args.min, args.max);
    args.angle = net->angle;
    args.aspect = net->aspect;
    args.exposure = net->exposure;
    args.saturation = net->saturation;
    args.hue = net->hue;
    args.size = net->w;

    args.paths = paths;
    args.classes = classes;
    args.n = imgs;
    args.m = N;
    args.labels = labels;
    if (tag){
        args.type = TAG_DATA;
    } else {
        args.type = CLASSIFICATION_DATA;
    }

    data train;
    data buffer;
    pthread_t load_thread;
    args.d = &buffer;
    load_thread = load_data(args);

    int count = 0;
    ///int epoch = (*net->seen)/N;
    ///while(get_current_batch(net) < net->max_batches || net->max_batches == 0)
    while(1)
    {
        if(net->random && count++%40 == 0){
            printf("Resizing\n");
            int dim = (rand() % 11 + 4) * 32;
            //if (get_current_batch(net)+200 > net->max_batches) dim = 608;
            //int dim = (rand() % 4 + 16) * 32;
            printf("%d\n", dim);
            args.w = dim;
            args.h = dim;
            args.size = dim;
            args.min = net->min_ratio*dim;
            args.max = net->max_ratio*dim;
            printf("%d %d\n", args.min, args.max);

            pthread_join(load_thread, 0);
            train = buffer;
            free_data(train);
            load_thread = load_data(args);

            for(i = 0; i < ngpus; ++i){
                resize_network(nets[i], dim, dim);
            }
            net = nets[0];
        }
        time = what_time_is_it_now();

        pthread_join(load_thread, 0);
        train = buffer;
        load_thread = load_data(args);  ///args.d == &buffer == &train

        printf("Loaded: %lf seconds\n", what_time_is_it_now()-time);
        time = what_time_is_it_now();

        float loss = 0;
#ifdef GPU
        if(ngpus == 1){
            loss = train_network(net, train);
        } else {
            loss = train_networks(nets, ngpus, train, 4);
        }
#else
        loss = train_network(net, train);
#endif
        if(avg_loss == -1) avg_loss = loss;
        avg_loss = avg_loss*.9 + loss*.1;
        printf("%d, %.3f: %f, %f avg, %f rate, %lf seconds, %d images\n"
            , get_current_batch(net), (float)(*net->seen)/N, loss, avg_loss, get_current_rate(net), what_time_is_it_now()-time, *net->seen);
        free_data(train);
        /**
        if(*net->seen/N > epoch){
            epoch = *net->seen/N;
            char buff[256];
            sprintf(buff, "%s/%s_%d.weights",backup_directory,base, epoch);
            save_weights(net, buff);
        }
        */
        ///if(get_current_batch(net)%1000 == 0){
        if(get_current_batch(net)%10 == 0){
            char buff[256];
            sprintf(buff, "%s/%s_%f.backup",backup_directory,base, avg_loss);
            save_weights(net, buff);
        }
        if (avg_loss < 0.001) break;
        if (avg_loss == INFINITY) goto _end;
    }
    char buff[256];
    sprintf(buff, "%s/%s.weights", backup_directory, base);
    save_weights(net, buff);

_end:
    pthread_join(load_thread, 0);
    free_network(net);
    if(labels) free_ptrs((void**)labels, classes);
    free_ptrs((void**)paths, plist->size);
    free_list(plist);
    free(base);
}


void train_classifier_batch(char *datacfg, char *cfgfile, char *weightfile, int *gpus, int ngpus, int clear)
{
    int i;
    float avg_loss = 0;
    char *base = basecfg(cfgfile);
    pr_info("base=%s, ngpus=%d\n", base, ngpus);
    network **nets = calloc(ngpus, sizeof(network*));

    srand(time(0));
    int seed = rand();
    for(i = 0; i < ngpus; ++i){
        srand(seed);
#ifdef GPU
        cuda_set_device(gpus[i]);
#endif
        nets[i] = load_network(cfgfile, weightfile, clear);
        nets[i]->learning_rate *= ngpus;
    }
    srand(time(0));
    network *net = nets[0];

    ///net->batch = 128 --> 1
    int imgs = net->batch * net->subdivisions * ngpus;

    printf("Learning Rate: %g, Momentum: %g, Decay: %g\n", net->learning_rate, net->momentum, net->decay);
    list *options = read_data_cfg(datacfg);

    char *backup_directory = option_find_str(options, "backup", "backup/");
    int tag = option_find_int_quiet(options, "tag", 0);
    char *label_list = option_find_str(options, "labels", "data/labels.lst");
    char *train_list = option_find_str(options, "train", "data/train.lst");

    ///char *tree = option_find_str(options, "tree", 0);
    ///if (tree) net->hierarchy = read_tree(tree);
    int classes = option_find_int(options, "classes", 2);

    char **labels = 0;
    if(!tag){
        labels = get_labels(label_list);
    }

    list *plist = get_paths(train_list);
    char **paths = (char **)list_to_array(plist);
    pr_info("plist->size:N: %d\n", plist->size);
    int n = plist->size;    ///train list size(rows)

    ///batch size
    for(i=0; i<n; i++)
        printf("paths[%d]: %s\n", i, paths[i]);

    load_args args = {0};
    args.w = net->w;
    args.h = net->h;
    ///args.threads = 32;
    args.threads = 1;
    args.hierarchy = net->hierarchy;

    args.min = net->min_ratio*net->w;
    args.max = net->max_ratio*net->w;
    pr_info("args.min=%d, args.max=%d\n", args.min, args.max);
    args.angle = net->angle;
    args.aspect = net->aspect;
    args.exposure = net->exposure;
    args.saturation = net->saturation;
    args.hue = net->hue;
    args.size = net->w;

    args.paths = paths;
    args.classes = classes;
    args.n = imgs;
    args.m = 0;  ///random paths
    ///args.center = 1;    ///resize

    args.labels = labels;
    args.type = (tag) ? TAG_DATA : CLASSIFICATION_DATA;

    float loss = 0;
    char fname[256];

    data buffer;
    args.d = &buffer;

    unsigned int cnt = 0;
    pr_info("net->batch=%d/%d, w=%d, h=%d\n", net->batch, n, net->w, net->h);
    ///for(i=0; i<n; i++)
    while(1)
    {
        cnt++;
        /*
        buffer = load_data_augment(paths, args.n, args.m, args.labels, args.classes
                    , args.hierarchy, args.min, args.max, args.size, args.angle, args.aspect
                    , args.hue, args.saturation, args.exposure, args.center);
        */
        buffer = load_data_paths(paths, args.n, args.m, labels, classes, net->hierarchy, net->w, net->h);

        pr_info("[%d] %s(): Start--------------------------------------\n", cnt, __FUNCTION__);
        loss = train_network(net, buffer);
        pr_info("[%d] %s(): End: Cost: %f\n", cnt, __FUNCTION__, loss);
        avg_loss += loss;

        if (loss == INFINITY) {
            pr_warn("loss is INFINITY.\n");
            goto _finish;
        }

        if (!(cnt % 10)) {
            avg_loss /= 10;
            pr_info("%s(): avg_loss=%f\n", __FUNCTION__, avg_loss);
            sprintf(fname, "%s/%s_%f.w", backup_directory, base, avg_loss);
            save_weights(net, fname);

            if (avg_loss < 0.001) goto _finish;
            avg_loss = 0;
        }
    } //while

_finish:
    free_network(net);
    if(labels) free_ptrs((void**)labels, classes);
    free_ptrs((void**)paths, plist->size);
    free_list(plist);
    free(base);
}


void validate_classifier_crop(char *datacfg, char *filename, char *weightfile)
{
    int i = 0;
    network *net = load_network(filename, weightfile, 0);
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *label_list = option_find_str(options, "labels", "data/labels.list");
    char *valid_list = option_find_str(options, "valid", "data/train.list");
    int classes = option_find_int(options, "classes", 2);
    int topk = option_find_int(options, "top", 1);

    char **labels = get_labels(label_list);
    list *plist = get_paths(valid_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    clock_t time;
    float avg_acc = 0;
    float avg_topk = 0;
    int splits = m/1000;
    int num = (i+1)*m/splits - i*m/splits;

    data val, buffer;

    load_args args = {0};
    args.w = net->w;
    args.h = net->h;

    args.paths = paths;
    args.classes = classes;
    args.n = num;
    args.m = 0;
    args.labels = labels;
    args.d = &buffer;
    args.type = OLD_CLASSIFICATION_DATA;

    pthread_t load_thread = load_data_in_thread(args);
    for(i = 1; i <= splits; ++i){
        time=clock();

        pthread_join(load_thread, 0);
        val = buffer;

        num = (i+1)*m/splits - i*m/splits;
        char **part = paths+(i*m/splits);
        if(i != splits){
            args.paths = part;
            load_thread = load_data_in_thread(args);
        }
        printf("Loaded: %d images in %lf seconds\n", val.X.rows, sec(clock()-time));

        time=clock();
        float *acc = network_accuracies(net, val, topk);
        avg_acc += acc[0];
        avg_topk += acc[1];
        printf("%d: top 1: %f, top %d: %f, %lf seconds, %d images\n", i, avg_acc/i, topk, avg_topk/i, sec(clock()-time), val.X.rows);
        free_data(val);
    }
}

void validate_classifier_10(char *datacfg, char *filename, char *weightfile)
{
    int i, j;
    network *net = load_network(filename, weightfile, 0);
    set_batch_network(net, 1);
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *label_list = option_find_str(options, "labels", "data/labels.list");
    char *valid_list = option_find_str(options, "valid", "data/train.list");
    int classes = option_find_int(options, "classes", 2);
    int topk = option_find_int(options, "top", 1);

    char **labels = get_labels(label_list);
    list *plist = get_paths(valid_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    float avg_acc = 0;
    float avg_topk = 0;
    int *indexes = calloc(topk, sizeof(int));

    for(i = 0; i < m; ++i){
        int class = -1;
        char *path = paths[i];
        for(j = 0; j < classes; ++j){
            if(strstr(path, labels[j])){
                class = j;
                break;
            }
        }
        int w = net->w;
        int h = net->h;
        int shift = 32;
        image im = load_image_color(paths[i], w+shift, h+shift);
        image images[10];
        images[0] = crop_image(im, -shift, -shift, w, h);
        images[1] = crop_image(im, shift, -shift, w, h);
        images[2] = crop_image(im, 0, 0, w, h);
        images[3] = crop_image(im, -shift, shift, w, h);
        images[4] = crop_image(im, shift, shift, w, h);
        flip_image(im);
        images[5] = crop_image(im, -shift, -shift, w, h);
        images[6] = crop_image(im, shift, -shift, w, h);
        images[7] = crop_image(im, 0, 0, w, h);
        images[8] = crop_image(im, -shift, shift, w, h);
        images[9] = crop_image(im, shift, shift, w, h);
        float *pred = calloc(classes, sizeof(float));
        for(j = 0; j < 10; ++j){
            float *p = network_predict(net, images[j].data);
            if(net->hierarchy) hierarchy_predictions(p, net->outputs, net->hierarchy, 1, 1);
            axpy_cpu(classes, 1, p, 1, pred, 1);
            free_image(images[j]);
        }
        free_image(im);
        top_k(pred, classes, topk, indexes);
        free(pred);
        if(indexes[0] == class) avg_acc += 1;
        for(j = 0; j < topk; ++j){
            if(indexes[j] == class) avg_topk += 1;
        }

        printf("%d: top 1: %f, top %d: %f\n", i, avg_acc/(i+1), topk, avg_topk/(i+1));
    }
}

void validate_classifier_full(char *datacfg, char *filename, char *weightfile)
{
    int i, j;
    network *net = load_network(filename, weightfile, 0);
    set_batch_network(net, 1);
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *label_list = option_find_str(options, "labels", "data/labels.list");
    char *valid_list = option_find_str(options, "valid", "data/train.list");
    int classes = option_find_int(options, "classes", 2);
    int topk = option_find_int(options, "top", 1);

    char **labels = get_labels(label_list);
    list *plist = get_paths(valid_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    float avg_acc = 0;
    float avg_topk = 0;
    int *indexes = calloc(topk, sizeof(int));

    int size = net->w;
    for(i = 0; i < m; ++i){
        int class = -1;
        char *path = paths[i];
        for(j = 0; j < classes; ++j){
            if(strstr(path, labels[j])){
                class = j;
                break;
            }
        }
        image im = load_image_color(paths[i], 0, 0);
        image resized = resize_min(im, size);
        resize_network(net, resized.w, resized.h);
        //show_image(im, "orig");
        //show_image(crop, "cropped");
        //cvWaitKey(0);
        float *pred = network_predict(net, resized.data);
        if(net->hierarchy) hierarchy_predictions(pred, net->outputs, net->hierarchy, 1, 1);

        free_image(im);
        free_image(resized);
        top_k(pred, classes, topk, indexes);

        if(indexes[0] == class) avg_acc += 1;
        for(j = 0; j < topk; ++j){
            if(indexes[j] == class) avg_topk += 1;
        }

        printf("%d: top 1: %f, top %d: %f\n", i, avg_acc/(i+1), topk, avg_topk/(i+1));
    }
}

void validate_classifier_multi(char *datacfg, char *cfg, char *weights)
{
    int i, j;
    network *net = load_network(cfg, weights, 0);
    set_batch_network(net, 1);
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *label_list = option_find_str(options, "labels", "data/labels.list");
    char *valid_list = option_find_str(options, "valid", "data/test.list");
    int classes = option_find_int(options, "classes", 2);
    int topk = option_find_int(options, "top", 1);

    char **labels = get_labels(label_list);
    list *plist = get_paths(valid_list);
    //int scales[] = {224, 288, 320, 352, 384};
    int scales[] = {224, 256, 288, 320};
    int nscales = sizeof(scales)/sizeof(scales[0]);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    float avg_acc = 0;
    float avg_topk = 0;
    int *indexes = calloc(topk, sizeof(int));

    for(i = 0; i < m; ++i){
        int class = -1;
        char *path = paths[i];
        for(j = 0; j < classes; ++j){
            if(strstr(path, labels[j])){
                class = j;
                break;
            }
        }
        float *pred = calloc(classes, sizeof(float));
        image im = load_image_color(paths[i], 0, 0);
        for(j = 0; j < nscales; ++j){
            image r = resize_max(im, scales[j]);
            resize_network(net, r.w, r.h);
            float *p = network_predict(net, r.data);
            if(net->hierarchy) hierarchy_predictions(p, net->outputs, net->hierarchy, 1 , 1);
            axpy_cpu(classes, 1, p, 1, pred, 1);
            flip_image(r);
            p = network_predict(net, r.data);
            axpy_cpu(classes, 1, p, 1, pred, 1);
            if(r.data != im.data) free_image(r);
        }
        free_image(im);
        top_k(pred, classes, topk, indexes);
        free(pred);
        if(indexes[0] == class) avg_acc += 1;
        for(j = 0; j < topk; ++j){
            if(indexes[j] == class) avg_topk += 1;
        }

        printf("%d: top 1: %f, top %d: %f\n", i, avg_acc/(i+1), topk, avg_topk/(i+1));
    }
}

void label_classifier(char *datacfg, char *filename, char *weightfile)
{
    int i;
    network *net = load_network(filename, weightfile, 0);
    set_batch_network(net, 1);
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *label_list = option_find_str(options, "names", "data/labels.list");
    char *test_list = option_find_str(options, "test", "data/train.list");
    int classes = option_find_int(options, "classes", 2);

    char **labels = get_labels(label_list);
    list *plist = get_paths(test_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    for(i = 0; i < m; ++i){
        image im = load_image_color(paths[i], 0, 0);
        image resized = resize_min(im, net->w);
        image crop = crop_image(resized, (resized.w - net->w)/2, (resized.h - net->h)/2, net->w, net->h);
        float *pred = network_predict(net, crop.data);

        if(resized.data != im.data) free_image(resized);
        free_image(im);
        free_image(crop);
        int ind = max_index(pred, classes);

        printf("%s\n", labels[ind]);
    }
}

void test_classifier(char *datacfg, char *cfgfile, char *weightfile, int target_layer)
{
    int curr = 0;
    network *net = load_network(cfgfile, weightfile, 0);
    srand(time(0));

    list *options = read_data_cfg(datacfg);

    char *test_list = option_find_str(options, "test", "data/test.list");
    int classes = option_find_int(options, "classes", 2);

    list *plist = get_paths(test_list);

    char **paths = (char **)list_to_array(plist);
    int m = plist->size;
    free_list(plist);

    clock_t time;

    data val, buffer;

    load_args args = {0};
    args.w = net->w;
    args.h = net->h;
    args.paths = paths;
    args.classes = classes;
    args.n = net->batch;
    args.m = 0;
    args.labels = 0;
    args.d = &buffer;
    args.type = OLD_CLASSIFICATION_DATA;

    pthread_t load_thread = load_data_in_thread(args);
    for(curr = net->batch; curr < m; curr += net->batch){
        time=clock();

        pthread_join(load_thread, 0);
        val = buffer;

        if(curr < m){
            args.paths = paths + curr;
            if (curr + net->batch > m) args.n = m - curr;
            load_thread = load_data_in_thread(args);
        }
        fprintf(stderr, "Loaded: %d images in %lf seconds\n", val.X.rows, sec(clock()-time));

        time=clock();
        matrix pred = network_predict_data(net, val);

        int i, j;
        if (target_layer >= 0){
            //layer l = net->layers[target_layer];
        }

        for(i = 0; i < pred.rows; ++i){
            printf("%s", paths[curr-net->batch+i]);
            for(j = 0; j < pred.cols; ++j){
                printf("\t%g", pred.vals[i][j]);
            }
            printf("\n");
        }

        free_matrix(pred);

        fprintf(stderr, "%lf seconds, %d images, %d total\n", sec(clock()-time), val.X.rows, curr);
        free_data(val);
    }
}

static float classifier_thread_train_run(char **paths, load_args *args)
{
    data buffer;
    float loss = 0;

    buffer = load_data_augment(paths
                , args->n, args->m, args->labels, args->classes, args->hierarchy
                , args->min, args->max, args->size, args->angle, args->aspect
                , args->hue, args->saturation, args->exposure, args->center);

    pr_info("net->batch=%d, X.cols=%d, y.cols=%d\n", args->net->batch, buffer.X.cols,  buffer.y.cols);

    pr_debug("train_network() start.\n");
    loss = train_network(args->net, buffer);
    pr_debug("train_network() end: loss=%f\n", loss);

    if (loss == INFINITY) loss = 100.0;
    return loss;
}

void* classifier_thread_train(void* arg)
{
    load_args* args = (load_args*)arg;
    char path[120], dst[120];
    struct dirent *entry;
    DIR *dir;
	///struct stat sb;
	float cost;
	unsigned int i, cnt = 0;
	static unsigned int TrainCount=0;

	char *paths[args->n];
	char *files[args->n];
	for (i=0; i < args->n; i++) {
        paths[i] = malloc(sizeof(path));
        files[i] = malloc(80);
    }

    while (1)
    {
        cnt = 0;
        dir = opendir (PATH_DATA_ML_TRAIN);
         while ((entry = readdir (dir)) != NULL)
        {
            ///stat(path, &sb);
            ///if ((int)sb.st_size > 0)
            if (entry->d_type == DT_REG) {
                sprintf(paths[cnt], "%s/%s", PATH_DATA_ML_TRAIN, entry->d_name);
                ///strcpy(files[cnt] , entry->d_name);
                sprintf(files[cnt] ,"%s",  entry->d_name);
                cnt++;
            }
            if (cnt >= args->n) break;
        }
        closedir(dir);
        usleep(20000);  //20ms
        if (cnt < args->n) continue;

        cost = classifier_thread_train_run(paths, args);
        if (cost < 0.1) {
            sprintf(path, "%s", PATH_DATA_ML_BAK0);    ///train OK
        } else {
            sprintf(path, "%s", PATH_DATA_ML_BAK1);
        }

        TrainCount++;
        sprintf(dst, "%s/%s_%d_%f.backup", args->path_backup, args->base, TrainCount, cost);
        save_weights(args->net, dst);

        for (i=0; i < args->n; i++) {
            sprintf(dst, "%s/%s", path, files[i]);
            if (rename(paths[i], dst) >= 0) unlink(paths[i]);   ///move and delete
        }

        usleep(40000);  //40ms
    } //while
}


///============================ stable functions ==============================

uint32_t MPlay1=0;
uint32_t MPlay2=0;
uint32_t MPlay3=0;

///@idx: 1: class
///@idx: 2: emotion
///@idx: 3: person
///@idx: 4: race

static void _classifier_ans_response(int idx, int ans, int mplay)
{
    int order[] = { 4, 2, 4, 4, 4, 2, 2, 4 };

    static char *voices[][10] = {
        {"_carpin_.mp3", "_eagles_.mp3", "_jungjj_.mp3", "_queen_.mp3", "_unk_.mp3"
            , "_unk_.mp3", "_unk_.mp3", "_unk_.mp3", "_unk_.mp3", "_unk_.mp3" }

        , {"_anger_.mp3", "_poker_.mp3", "_poker_.mp3", "_sad_.mp3", "_scare_.mp3"
            , "_smile_.mp3", "_smile_.mp3", "_surpr_.mp3", "_unk_.mp3", "_unk_.mp3" }

        , {"_baby_.mp3", "_beard_.mp3", "_black_.mp3", "_child_.mp3", "_adult_.mp3"
            , "_adult_.mp3", "_older_.mp3", "_teen_.mp3", "_unk_.mp3", "_unk_.mp3" }

        , {"_black_m_.mp3", "_black_w_.mp3", "_east_m_.mp3", "_east_m_.mp3", "_east_w_.mp3"
            , "_east_w_.mp3", "_west_m_.mp3", "_west_m_.mp3", "_west_w_.mp3", "_west_w_.mp3" }

        , {"_carpin_.mp3", "_eagles_.mp3", "_jungjj_.mp3", "_queen_.mp3", "_unk_.mp3"
            , "_unk_.mp3", "_unk_.mp3", "_unk_.mp3", "_unk_.mp3", "_unk_.mp3" }

        , {"_anger_.mp3", "_poker_.mp3", "_poker_.mp3", "_sad_.mp3", "_scare_.mp3"
            , "_smile_.mp3", "_smile_.mp3", "_surpr_.mp3", "_unk_.mp3", "_unk_.mp3" }

        , {"_baby_.mp3", "_beard_.mp3", "_black_.mp3", "_black_.mp3", "_glass_.mp3"
            , "_adult_.mp3", "_adult_.mp3", "_adult_.mp3", "_older_.mp3", "_teen_.mp3" }

        , {"_black_m_.mp3", "_black_w_.mp3", "_east_m_.mp3", "_east_w_.mp3", "_west_m_.mp3"
            , "_west_m_.mp3", "_west_w_.mp3", "_unk_.mp3", "_unk_.mp3", "_unk_.mp3" }
        };

    if (idx <= 0) {
        pr_info("  ==>> _unk_\n");
        return;
    }
    if (idx > sizeof(order) / sizeof(order[0])) return;

    idx -= 1;
    ans /= order[idx];
    pr_info(" ==>> %s\n", voices[idx][ans]);

    if (mplay && RunCfg.mplay)
    {
        char command[80];
        sprintf(command, "mplayer -really-quiet %s%s &", PATH_FACE_VOICES, voices[idx][ans]);
        system(command);
        usleep(1500000);  ///2s

        if (!MPlay1 && !strncmp(voices[idx][ans], "_carpin_", 8)) {
            if (MPlay2+MPlay3 > 0) system ("killall mplayer");
            MPlay2=0; MPlay3=0;
            system("mplayer -really-quiet ../data/music/Carpenters_OST_Top_Of_The_World.mp3 &");
            usleep(20000);  ///20ms
            MPlay1++;
        } else if (!MPlay2 && !strncmp(voices[idx][ans], "_eagles_", 8)) {
            if (MPlay1+MPlay3 > 0) system ("killall mplayer");
            MPlay1=0; MPlay3=0;
            system("mplayer -really-quiet ../data/music/Eagles_Hotel_California.mp3 &");
            usleep(20000);  ///20ms
            MPlay2++;
        } else if (!MPlay3 && !strncmp(voices[idx][ans], "_queen_", 7)) {
            if (MPlay1+MPlay2 > 0) system ("killall mplayer");
            MPlay1=0; MPlay2=0;
            system("mplayer -really-quiet ../data/music/Queen-IWantToBreakFree.mp3 &");
            usleep(20000);  ///20ms
            MPlay3++;
        }
    }

}

static int _classifier_request_server(char* src, char* fname)
{
    char dst[160], ip[16];
    int start, end, ret, err=0;
    int i;

    if (RunCfg.ncnt > NET_MAX) {
        pr_err_msg("Server Count(%d > %d) Error!\n\n", RunCfg.ncnt, NET_MAX);
        return -1;
    }

    start = RunCfg.start + 1;
    if (strncmp(fname, "s086_", 5)==0) {
        end = start + RunCfg.step;
    } else {
        start = start + RunCfg.step;
        end = RunCfg.ncnt + 1;
    }
    pr_info("Request Server [start=%d, cnt=%d] -----------------------------------\n"
                        , start, end-start);

    for (i=start; i < end; i++)
    {
        sprintf(ip, "%s%d", RunCfg.net, i);
        ret = net_client (ip, RunCfg.port, NET_START);
        if (ret < 0) {
            pr_err("network error on %s\n", ip);
            err++;
        } else {
            sprintf(dst, "cp %s /mnt/cm%02d/ml/save/%s", src, i, fname); ///copy to server via NFS
            system(dst);
            usleep(10000);  //10ms
        }
    }
    system("sync");

    return err;
}

static int _classifier_wait_server(void)
{
    int ret, sum;
    int id[NET_MAX], ans[NET_MAX], max[NET_MAX], idx[NET_MAX], cnt[NET_MAX];
    char ip[16];
    unsigned int i;
    int id2=0, ans2=0, max2=0;

    if (RunCfg.ncnt > NET_MAX) {
        pr_err_msg("Server Count(%d > %d) Error!\n\n", RunCfg.ncnt, NET_MAX);
        return -1;
    }

    for (i=RunCfg.start; i<RunCfg.ncnt; i++) {
        max[i] = 0;
        cnt[i] = NET_RETRY;
    }

    do
    {
        usleep(10000);  ///10ms
        for (i=RunCfg.start; i<RunCfg.ncnt; i++)
        {
            if (cnt[i]==0) continue;

            sprintf(ip, "%s%d", RunCfg.net, i+1);
            ret = net_client (ip, RunCfg.port, NET_REQUEST);
            if (ret < 0) {
                cnt[i]  = 0;
                continue;
            }
            ans[i] = ret % 100000;
            if (ans[i] > max[i]) {
                max[i] = ans[i];
                idx[i] = (ret / 100000) % 100;
                id[i] = ret / 10000000;
            }
            cnt[i] = (ret == 0) ? --(cnt[i]) : 0;
            usleep(10000);  ///10ms
        }
        sum = 0;
        for (i=RunCfg.start; i<RunCfg.ncnt; i++) sum += cnt[i];

    } while(sum);

    for (i=RunCfg.start; i<RunCfg.ncnt; i++) {
        sprintf(ip, "%s%d", RunCfg.net, i+1);
        pr_info_msg("%02d: CM[%02d][%s] Answer=[%02d], Accuracy=%03.2f(%%) "
                                , i+1, id[i]+1, ip, idx[i], (float)max[i]/100);
        _classifier_ans_response(id[i], idx[i], (max[i] >= RunCfg.acc));

        if (max[i] > max2) {
            max2 = max[i];
            ans2 = idx[i];
            id2 = id[i];
        }
    }

    pr_info("*Final Result Accuracy=%03.2f(%%) ", (float)max2/100);
    _classifier_ans_response(id2, ans2, (max2 >= RunCfg.acc));

    return 0;
}

static void _classifier_thread_server_run(char* path, load_args* args, char* fname)
{
    int i;
    float ans=0;
    char dst[160];
    clock_t time;
    static unsigned int cnt=0;
    int *indexes = calloc(args->top, sizeof(int));

    pr_info("%s(): Start %s[%d] -----------------------------------\n"
                        , __FUNCTION__, args->base, ++cnt);
    time = clock();

    data din = load_data_simple(path, args->labels, args->classes
                                , args->net->hierarchy, args->net->w, args->net->h);
    float *out = network_predict(args->net, din.X.vals[0]);

    top_k(out, args->net->outputs, args->top, indexes);

    pr_info("%s: Predicted in %lf seconds.\n", path, sec(clock()-time));
    for(i = 0; i < args->top; ++i) {
        int idx = indexes[i];
        ans = out[idx]*100;

        pr_info("CM[%02d][%s] Answer=[%02d]%s, Accuracy=%03.2f(%%)\n"
                        ,  RunCfg.id, RunCfg.ip, idx, args->labels[idx], ans);
        if (i==0) {
            ans *= 100;
            NetMlResult = RunCfg.id * 10000000 + idx * 100000 + (ans + 0.5);

            if (ans >= RunCfg.acc) {
                sprintf(dst, "%s/%s_%s", PATH_DATA_ML_BAK0, args->labels[idx], fname);
                pr_info("[Correct] Move to Backup. Result=%u\n", NetMlResult);
            } else {
                sprintf(dst, "%s/%s", PATH_DATA_ML_TRAIN, fname);
                pr_info("[Unknown] Move to Tain. Result=%u\n", NetMlResult);
            }
            if (rename(path, dst) >= 0) unlink(path);   ///move and delete

            if (!RunCfg.save) {
                unlink(dst);
                usleep(10000);  //10ms
            }
            system("sync");
        }
    } //for

    if (din.X.vals) free(din.X.vals);

    //pr_info("%s(): End.\n\n", __FUNCTION__);
}

void* classifier_thread_server(void* arg)
{
    load_args* args = (load_args*)arg;
    char path[120];
    struct dirent *entry;
    DIR *dir;
	///struct stat sb;

    while (1)
    {
        dir = opendir (PATH_DATA_ML_SAVE);
        while ((entry = readdir (dir)) != NULL)
        {
            ///if (strlen(entry->d_name) > 5)
            ///stat(path, &sb);
            ///if ((int)sb.st_size > 0)
            if (entry->d_type == DT_REG && NetMlResult==0) {
                sprintf(path, "%s/%s", PATH_DATA_ML_SAVE, entry->d_name);
                _classifier_thread_server_run(path, args, entry->d_name);
            } //if
            usleep(20000);  //20ms
        } //while

        closedir(dir);
        usleep(40000);  //40ms
    } //while
}

static void _classifier_thread_local_run(char *src, load_args* args)
{
    int i, idx;
    clock_t time;
    static unsigned int cnt=0;

    int *indexes = calloc(args->top, sizeof(int));

    pr_info("Local[%s] Start %s[%d] -----------------------------------\n"
                        , RunCfg.name, args->base, ++cnt);
    time = clock();

    data din = load_data_simple(src, args->labels, args->classes
                                , args->net->hierarchy, args->net->w, args->net->h);
    float *out = network_predict(args->net, din.X.vals[0]);

    top_k(out, args->net->outputs, args->top, indexes);

    pr_info("%s: Predicted in %lf seconds.\n", src, sec(clock()-time));
    for(i = 0; i < args->top; ++i) {
        idx = indexes[i];
        pr_info("%02d:Local[%s] Answer=[%02d]%s, Accuracy=%03.2f(%%) "
                        ,  i, RunCfg.name, idx, args->labels[idx], out[idx]*100);
        _classifier_ans_response(RunCfg.id, idx, (out[idx] >= RunCfg.acc));
    } //for

    if (din.X.vals) free(din.X.vals);

    //pr_info("%s(): End.\n\n", __FUNCTION__);
    //pr_info("Local Me[%s] End. -----------------------------------\n", RunCfg.name);
}


void* classifier_thread_local(void* arg)
{
    load_args* args = (load_args*)arg;
    struct dirent *entry;
    DIR *dir;
    char src[80], dst[160];
    clock_t ctime;
    time_t rtime;
    unsigned int opcode = (RunCfg.opcode+1) << CONFIG_OP_SHIFT;
    int ret;
	///struct stat sb;

    while (1)
    {
        dir = opendir (PATH_DATA_ML_SAVE);
        while ((entry = readdir (dir)) != NULL)
        {
            ///if (strlen(entry->d_name) > 5)
            ///stat(path, &sb);
            ///if ((int)sb.st_size > 0)
            if (entry->d_type == DT_REG)
            {
                sprintf(src, "%s/%s", PATH_DATA_ML_SAVE, entry->d_name);
                ctime = clock();
                rtime = time(NULL);
                FaceProcessing++;

                if (opcode & CONFIG_OP_ME)  {
                    _classifier_thread_local_run(src, args);
                }

                if (opcode & CONFIG_OP_YOU) {
                    ret = _classifier_request_server(src, entry->d_name);
                    if (ret >= 0)  _classifier_wait_server();
                }

                pr_info("%s: RunTime is %d(%f) seconds.\n\n"
                                , src, time(NULL)-rtime, sec(clock()-ctime));

                sprintf(dst, "%s/%s", PATH_DATA_ML_BAK0, entry->d_name);
                if (rename(src, dst) >= 0) unlink(src);   ///move and delete
                if (!RunCfg.save) {
                    unlink(dst);
                    usleep(10000);  //10ms
                }
                 system("sync");
            } //if
            usleep(10000);  //10ms
        } //while

        closedir(dir);
        FaceProcessing = 0;
        usleep(20000);  //20ms
    } //while
}

/**
    @type: running type
    @datfile: type.dat
    @cfgfile: type.cfg
    @wtsfile: type.wts
*/
void classifier_thread_run(char *datfile , char *cfgfile, char *wtsfile)
{
    network *net = load_network(cfgfile, wtsfile, 0);
    ///set_batch_network(net, 1);   ///net->batch

    //int imgs = net->batch * net->subdivisions * ngpus;

    list *options = read_data_cfg(datfile);
    //char *valid_list = option_find_str(options, "valid", "data/test.list");
    char *label_list = option_find_str(options, "labels", "data/labels.lst");
    char *backup = option_find_str(options, "backup", "backup/");
    int classes = option_find_int(options, "classes", 10);  ///input quantity
    int topk = option_find_int(options, "top", 1);          ///output(answer) quantity
    char **labels = get_labels(label_list);

    load_args args = {0};
    args.net = net;
    args.w = net->w;
    args.h = net->h;
    args.threads = 32;
    args.min = net->min_ratio*net->w;
    args.max = net->max_ratio*net->w;
    args.size = net->w;
    args.classes = classes;
    args.n = 1;
    args.m = 0;
    ///args.center = 1;    ///resize
    ///args.type = (tag) ? TAG_DATA : CLASSIFICATION_DATA;
    args.type = CLASSIFICATION_DATA;

    args.top = topk;

    args.base = RunCfg.type;
    args.path_backup = backup;

    args.labels = labels;

    pr_info("---------------------- %s Infomations --------------------\n", __FUNCTION__);
    pr_info("target:                %s\n", RunCfg.target);
    pr_info("type:                  %s\n", RunCfg.type);
    pr_info("amount:                %d\n", args.n);
    pr_info("classes:               %d\n", classes);
    pr_info("net->batch:            %d\n", net->batch);
    pr_info("net->learning_rate:    %f\n", net->learning_rate);
    pr_info("net->w:                %d\n", net->w);
    pr_info("net->h:                %d\n", net->h);

	pthread_t  tid1, tid2, tid3;
	unsigned int who = (RunCfg.who+1) << CONFIG_WHO_SHIFT;
	unsigned int opcode = (RunCfg.opcode+1) << CONFIG_OP_SHIFT;

	if (who & CONFIG_WHO_LOCAL) {
        if (pthread_create(&tid1, NULL, classifier_thread_local, &args)) {
            pr_err("can't create classifier_thread_local()\n");
        } else {
            pr_info("classifier_thread_local(%d) started.\n", (int)tid1);
        }
    }

    if (who & CONFIG_WHO_SERVER) {
        if (pthread_create(&tid2, NULL, classifier_thread_server, &args)) {
            pr_err("can't create classifier_thread_server()\n");
        } else {
            pr_info("classifier_thread_server(%d) started.\n", (int)tid2);
        }
    }

    if (opcode & CONFIG_OP_TRAIN) {
        if (pthread_create(&tid3, NULL, classifier_thread_train, &args)) {
            pr_err("can't create classifier_thread_train()\n");
        } else {
            pr_info("classifier_thread_train(%d) started.\n", (int)tid3);
        }
    }

	while(1) pause();

    free_network(net);
    if(labels) free_ptrs((void**)labels, classes);

    pr_func_e;
}

/**
    @type: running type
    @datfile: type.dat
    @cfgfile: type.cfg
    @wtsfile: type.wts
*/
void classifier_train_run(char *datfile, char *cfgfile, char *wtsfile)
{
    network *net = load_network(cfgfile, wtsfile, 0);

    //input image quantity
    //int imgs = net->batch * net->subdivisions * ngpus;

    list *options = read_data_cfg(datfile);
    char *train_list = option_find_str(options, "train", "data/train.lst");
    char *label_list = option_find_str(options, "labels", "data/labels.lst");
    char *backup = option_find_str(options, "backup", "backup/");
    int classes = option_find_int(options, "classes", 10);  ///output answer quantity
    int tag = option_find_int_quiet(options, "tag", 0);

    //char *tree = option_find_str(options, "tree", 0);
    //if (tree) net->hierarchy = read_tree(tree);

    char **labels = 0;
    if(!tag) labels = get_labels(label_list);

    list *plist = get_paths(train_list);
    char **paths = (char **)list_to_array(plist);
    int n = plist->size;    ///train list count
    free_list(plist);

    //load_args args = {0};
    //args.type = (tag) ? TAG_DATA : CLASSIFICATION_DATA;

    float cost, avg_cost = 0.0;
    int i = 0;
    unsigned int cnt = 0;

    pr_info("---------------------- %s Infomations --------------------\n", __FUNCTION__);
    pr_info("target:                %s\n", RunCfg.target);
    pr_info("type:                  %s\n", RunCfg.type);
    pr_info("amount:                %d\n", n);
    pr_info("classes:               %d\n", classes);
    pr_info("net->batch:            %d\n", net->batch);
    pr_info("net->learning_rate:    %f\n", net->learning_rate);
    pr_info("net->w:                %d\n", net->w);
    pr_info("net->h:                %d\n", net->h);

    for(i=0; i<n; i++)
        pr_info("paths[%d]: %s\n", i, paths[i]);

    data din = load_data_paths(paths, n, 0, labels, classes, net->hierarchy, net->w, net->h);

    pr_info("---------------------- %s Starting -----------------------\n", __FUNCTION__);

    ///for(cnt=0; cnt<CONFIG_TRAIN_AMOUNT; )
    while(1)
    {
        i = cnt % n;
        pr_debug("%u: %d: %s\n", cnt, i, paths[i]);
        //data din = load_data_simple(paths[i], labels, classes, net->hierarchy, net->w, net->h);
        memcpy(net->input, din.X.vals[i], din.X.cols*sizeof(float));
        memcpy(net->truth, din.y.vals[i], din.y.cols*sizeof(float));

        cost = train_network_datum(net);
        pr_debug("%u: cost=%f\n", cnt, cost);
        avg_cost += cost;

        if (cost == INFINITY) {
            pr_warn("cost is Infinity.[FAIL]\n");
            break;
        }

        cnt++;
        if (!(cnt % CONFIG_TRAIN_AMOUNT)) {
            char fname[256];
            avg_cost /= CONFIG_TRAIN_AMOUNT;
            pr_info("%u: avg_cost=%f\n", cnt, avg_cost);
            sprintf(fname, "%s/%s_%f.w", backup, RunCfg.type, avg_cost);
            save_weights(net, fname);

            if (avg_cost < CONFIG_TRAIN_FINISH_COST) {
                pr_info("%s() Completed[avg_cost=%f]\n", __FUNCTION__, avg_cost);
                break;
            }
            avg_cost = 0;
        }
    } //while

    ///free_network(net);
    ///if(labels) free_ptrs((void**)labels, classes);
    ///if(paths) free_ptrs((void**)paths, plist->size);
}


void classifier_validate_run(char *datfile, char *cfgfile, char *wtsfile)
{
    network *net = load_network(cfgfile, wtsfile, 0);
    ///set_batch_network(net, 1);   ///net->batch

    list *options = read_data_cfg(datfile);
    char *valid_list = option_find_str(options, "valid", "data/test.list");
    char *label_list = option_find_str(options, "labels", "data/labels.lst");
    int classes = option_find_int(options, "classes", 10);  ///input quantity
    int topk = option_find_int(options, "top", 1);          ///output(answer) quantity

    //char *backup = option_find_str(options, "backup", "backup/");
    //int tag = option_find_int_quiet(options, "tag", 0);
    //char *leaf_list = option_find_str(options, "leaves", 0);
    //if(leaf_list) change_leaves(net->hierarchy, leaf_list);

    char **labels = get_labels(label_list);
    list *plist = get_paths(valid_list);

    char **paths = (char **)list_to_array(plist);
    int n = plist->size;    ///valid list count
    free_list(plist);

    float avg_acc = 0.0;
    float avg_topk = 0.0;
    int *indexes = calloc(topk, sizeof(int));

    int i, j;

    pr_info("---------------------- %s Infomations --------------------\n", __FUNCTION__);
    pr_info("target:                %s\n", RunCfg.target);
    pr_info("type:                  %s\n", RunCfg.type);
    pr_info("amount:                %d\n", n);
    pr_info("classes:               %d\n", classes);
    pr_info("net->batch:            %d\n", net->batch);
    pr_info("net->learning_rate:    %f\n", net->learning_rate);
    pr_info("net->w:                %d\n", net->w);
    pr_info("net->h:                %d\n", net->h);

    for(i=0; i<n; i++)
        pr_info("paths[%d]: %s\n", i, paths[i]);

    data din = load_data_paths(paths, n, 0, labels, classes, net->hierarchy, net->w, net->h);

    pr_info("---------------------- %s Starting -----------------------\n", __FUNCTION__);

    for(i = 0; i < n; ++i)
    {
        int ans = -1;
        for(j = 0; j < classes; ++j){
            if(strstr(paths[i], labels[j])){
                ans = j;
                break;
            }
        }

        pr_debug("%d: %s\n", i, paths[i]);
        ///data din = load_data_simple(paths[i], labels, classes, net->hierarchy, net->w, net->h);
        ///memcpy(net->input, din.X.vals[i], din.X.cols*sizeof(float));

        float *out = network_predict(net, din.X.vals[i]);
        top_k(out, classes, topk, indexes);

        int idx = indexes[0] ;
        if(idx == ans) {
            avg_acc += 1;
            pr_info("Correct: answer[%d==%d]=%s, accuracy=%f\n", ans, idx, labels[idx], out[idx]);
        } else {
            pr_info("Wrong: answer[%d!=%d]=%s, accuracy=%f\n", ans, idx, labels[idx], out[idx]);
        }
        for(j = 0; j < topk; ++j){
            if(indexes[j] == ans) avg_topk += 1;
        }

        pr_info("%d: avg_accuracy: top[1]=%f, topk[%d]=%f\n\n", i, avg_acc/(i+1), topk, avg_topk/(i+1));
    }

    ///free_network(net);
    ///if(labels) free_ptrs((void**)labels, classes);
    ///if(paths) free_ptrs((void**)paths, plist->size);
}

// Benchmark CLI for section 5: runs each algorithm on several image sizes and writes a CSV.
//
//   seam_bench [--image path] [--out file.csv] [--seams N] [--sizes 100,200,400]
//              [--algos dp,greedy,graphcut] [--energy gradient|sobel] [--save-images]
//
// Graph cut runs one max-flow per seam, so keep sizes small (100-200) for it.
//
// Always benchmark the Release build.

#include <seam/Carver.h>
#include <seam/SeamFinder.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Args {
    std::string                 image = std::string(SEAM_ASSETS_DIR) + "/images/sample.jpg";
    std::string                 out   = std::string(SEAM_RESULTS_DIR) + "/bench.csv";
    int                         seams = 50;
    std::vector<int>            sizes = {100, 200, 400};
    std::vector<seam::Algorithm> algos = {seam::Algorithm::DP, seam::Algorithm::Greedy,
                                          seam::Algorithm::GraphCut};
    seam::EnergyType            energy     = seam::EnergyType::Gradient;
    bool                        saveImages = false;
};

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> parts;
    std::stringstream        ss(s);
    for (std::string p; std::getline(ss, p, sep);)
        if (!p.empty())
            parts.push_back(p);
    return parts;
}

Args parseArgs(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        const std::string k    = argv[i];
        auto              next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--image") a.image = next();
        else if (k == "--out") a.out = next();
        else if (k == "--seams") a.seams = std::stoi(next());
        else if (k == "--sizes") {
            a.sizes.clear();
            for (const auto& p : split(next(), ',')) a.sizes.push_back(std::stoi(p));
        } else if (k == "--algos") {
            a.algos.clear();
            for (const auto& p : split(next(), ',')) {
                if (p == "dp") a.algos.push_back(seam::Algorithm::DP);
                else if (p == "greedy") a.algos.push_back(seam::Algorithm::Greedy);
                else if (p == "graphcut") a.algos.push_back(seam::Algorithm::GraphCut);
            }
        } else if (k == "--energy") {
            a.energy = next() == "sobel" ? seam::EnergyType::Sobel : seam::EnergyType::Gradient;
        } else if (k == "--save-images") {
            a.saveImages = true;
        } else {
            std::cerr << "Unknown argument: " << k << "\n";
        }
    }
    return a;
}

} // namespace

int main(int argc, char** argv) {
    const Args args = parseArgs(argc, argv);

    const cv::Mat source = cv::imread(args.image, cv::IMREAD_COLOR);
    if (source.empty()) {
        std::cerr << "Could not load " << args.image << "\n";
        return 1;
    }

    // A bare file name has no parent directory, and create_directories("") throws.
    if (const fs::path dir = fs::path(args.out).parent_path(); !dir.empty())
        fs::create_directories(dir);
    std::ofstream csv(args.out);
    csv << "algorithm,energy,width,height,seams,ms_energy,ms_find,ms_remove,ms_total,"
           "total_seam_cost\n";

    for (const int size : args.sizes) {
        // Plain (non content-aware) scaling just to generate test inputs of each size.
        const double scale = static_cast<double>(size) / std::max(source.cols, source.rows);
        cv::Mat      img;
        cv::resize(source, img, {}, scale, scale, cv::INTER_AREA);
        const int seams = std::min(args.seams, img.cols - 1);

        for (const auto algo : args.algos) {
            seam::Carver carver(seam::makeSeamFinder(algo), args.energy);
            const cv::Mat result = carver.removeSeams(img, seams, seam::Orientation::Vertical);
            const auto&   s      = carver.stats();

            csv << seam::toString(algo) << ',' << seam::toString(args.energy) << ','
                << img.cols << ',' << img.rows << ',' << s.seamsRemoved << ',' << s.msEnergy
                << ',' << s.msFind << ',' << s.msRemove << ',' << s.msTotal() << ','
                << s.totalSeamCost << '\n';
            std::cout << seam::toString(algo) << "  " << img.cols << 'x' << img.rows << "  "
                      << s.seamsRemoved << " seams  " << s.msTotal() << " ms\n";

            if (args.saveImages) {
                const fs::path dir = fs::path(args.out).parent_path();
                cv::imwrite((dir / (std::string(seam::toString(algo)) + "_" +
                                    std::to_string(size) + ".png")).string(),
                            result);
            }
        }
    }

    std::cout << "Wrote " << args.out << "\n";
    return 0;
}

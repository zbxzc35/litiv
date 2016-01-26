
// This file is part of the LITIV framework; visit the original repository at
// https://github.com/plstcharles/litiv for more information.
//
// Copyright 2015 Pierre-Luc St-Charles; pierre-luc.st-charles<at>polymtl.ca
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#define DATASETUTILS_VIDEOSEGM_OUTOFSCOPE_VAL  uchar(85)
#define DATASETUTILS_VIDEOSEGM_UNKNOWN_VAL     uchar(170)
#define DATASETUTILS_VIDEOSEGM_SHADOW_VAL      uchar(50)

#define TNoGroup false
#define TGroup true

#include "litiv/utils/ParallelUtils.hpp"
#include "litiv/utils/PlatformUtils.hpp"
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

namespace litiv {

    enum eDatasetTypeList {
        eDatasetType_VideoSegm,
        eDatasetType_VideoRegistr,
        eDatasetType_ImageSegm,
        eDatasetType_ImageEdgDet,
        // ...
    };

    enum eDatasetList {

        //// VIDEO SEGMENTATION
        eDataset_VideoSegm_CDnet,
        eDataset_VideoSegm_Wallflower,
        eDataset_VideoSegm_PETS2001D3TC1,
        //eDataset_VideoSegm_...
        eDataset_VideoSegm_Custom,

        //// VIDEO REGISTRATION
        eDataset_VideoReg_LITIV2012b,
        //eDataset_VideoReg_...
        eDataset_VideoReg_Custom,

        //// IMAGE SEGMENTATION
        eDataset_ImageSegm_BSDS500,
        //eDataset_ImageSegm_...
        eDataset_ImageSegm_Custom,

        //// IMAGE EDGE DETECTION
        eDataset_ImageEdgDet_BSDS500,
        //eDataset_ImageEdgDet_...
        eDataset_ImageEdgDet_Custom

    };

    struct IDataHandler;
    using IDataHandlerPtr = std::shared_ptr<IDataHandler>;
    using IDataHandlerPtrArray = std::vector<IDataHandlerPtr>;
    using IDataHandlerPtrQueue = std::priority_queue<IDataHandlerPtr,IDataHandlerPtrArray,std::function<bool(const IDataHandlerPtr&,const IDataHandlerPtr&)>>;

    struct IDataset {
        virtual const std::string& getDatasetName() const = 0;
        virtual const std::string& getDatasetRootPath() const = 0;
        virtual const std::string& getResultsRootPath() const = 0;
        virtual const std::string& getResultsNamePrefix() const = 0;
        virtual const std::string& getResultsNameSuffix() const = 0;
        virtual const std::vector<std::string>& getWorkBatchPaths() const = 0;
        virtual const std::vector<std::string>& getSkippedNameTokens() const = 0;
        virtual const std::vector<std::string>& getGrayscaleNameTokens() const = 0;
        virtual size_t getOutputIdxOffset() const = 0;
        virtual double getScaleFactor() const = 0;
        virtual bool isSavingResults() const = 0;
        virtual bool is4ByteAligned() const = 0;
        virtual ~IDataset() = default;

        virtual size_t getTotPackets() const = 0;
        virtual double getProcessTime() const = 0;
        virtual double getExpectedLoad() const = 0;
        virtual size_t getProcessedPacketsCountPromise() = 0;
        virtual size_t getProcessedPacketsCount() const = 0;

        virtual void parseDataset() = 0;
        virtual void writeEvalReport() const = 0;
        virtual IDataHandlerPtrArray getBatches() const = 0;
        virtual IDataHandlerPtrQueue getSortedBatches() const = 0;
    };

    struct IDataHandler {
        virtual const std::string& getName() const = 0;
        virtual const std::string& getPath() const = 0;
        virtual const std::string& getResultsPath() const = 0;
        virtual const std::string& getRelativePath() const = 0;
        virtual double getExpectedLoad() const = 0;
        virtual size_t getTotPackets() const = 0;
        virtual bool isGrayscale() const = 0;
        virtual bool isBare() const = 0;
        virtual bool isGroup() const = 0;
        virtual IDataHandlerPtrArray getBatches() const = 0; // @@@@@ rename batches at dataset level to something else?
        virtual const IDataset& getDatasetInfo() const = 0;
        virtual eDatasetTypeList getDatasetType() const = 0;
        virtual eDatasetList getDataset() const = 0;
        virtual std::string writeInlineEvalReport(size_t nIndentSize, size_t nCellSize=12) const = 0;
        virtual void writeEvalReport() const = 0;
        virtual void parseDataset() = 0;
        virtual ~IDataHandler() = default;

        virtual void startPrecaching(bool bPrecacheGT, size_t nSuggestedBufferSize=SIZE_MAX) = 0; // starts prefetching data packets
        virtual void stopPrecaching() = 0; // stops prefetching data packets (for work batches, is also called in stopProcessing)
        virtual double getProcessTime() const = 0; // returns the current (or final) duration elapsed between start/stopProcessing calls
        virtual size_t getProcessedPacketsCountPromise() = 0;
        virtual size_t getProcessedPacketsCount() const = 0;

        template<typename Tp> static typename std::enable_if<std::is_base_of<IDataHandler,Tp>::value,bool>::type compare(const std::shared_ptr<Tp>& i, const std::shared_ptr<Tp>& j) {return PlatformUtils::compare_lowercase(i->getName(),j->getName());}
        template<typename Tp> static typename std::enable_if<std::is_base_of<IDataHandler,Tp>::value,bool>::type compare_load(const std::shared_ptr<Tp>& i, const std::shared_ptr<Tp>& j) {return i->getExpectedLoad()<j->getExpectedLoad();}
        static bool compare(const IDataHandler* i, const IDataHandler* j) {return PlatformUtils::compare_lowercase(i->getName(),j->getName());}
        static bool compare_load(const IDataHandler* i, const IDataHandler* j) {return i->getExpectedLoad()<j->getExpectedLoad();}
        static bool compare(const IDataHandler& i, const IDataHandler& j) {return PlatformUtils::compare_lowercase(i.getName(),j.getName());}
        static bool compare_load(const IDataHandler& i, const IDataHandler& j) {return i.getExpectedLoad()<j.getExpectedLoad();}
    protected:
        virtual IDataHandlerPtr getBatch(size_t& nPacketIdx) const = 0; // will throw if out of range or if handler is not a group, and readjust nPacketIdx for returned batch range otherwise
    };

    void writeOnImage(cv::Mat& oImg, const std::string& sText, const cv::Scalar& vColor, bool bBottom=false);
    cv::Mat getDisplayImage(const cv::Mat& oInputImg, const cv::Mat& oDebugImg, const cv::Mat& oSegmMask, size_t nIdx, cv::Point oDbgPt=cv::Point(-1,-1), cv::Size oRefSize=cv::Size(-1,-1));
    void validateKeyPoints(const cv::Mat& oROI, std::vector<cv::KeyPoint>& voKPs);

    struct DataPrecacher {
        // @@@@ rewrite to allow streaming with no limit? (might just need to modify init and set tot=inf)
        // @@@@ current impl expects all packets to be the same size
        DataPrecacher(const std::function<const cv::Mat&(size_t)>& lCallback);
        virtual ~DataPrecacher();
        const cv::Mat& getPacket(size_t nIdx);
        bool startPrecaching(size_t nTotPacketCount, size_t nSuggestedBufferSize);
        void stopPrecaching();
    private:
        void precache();
        const cv::Mat& getPacket_internal(size_t nIdx);
        const std::function<const cv::Mat&(size_t)>& m_lCallback;
        std::thread m_hPrecacher;
        std::mutex m_oSyncMutex;
        std::condition_variable m_oReqCondVar;
        std::condition_variable m_oSyncCondVar;
        bool m_bIsPrecaching;
        size_t m_nBufferSize;
        size_t m_nPacketCount;
        std::deque<cv::Mat> m_qoCache;
        std::vector<uchar> m_vcBuffer;
        size_t m_nFirstBufferIdx;
        size_t m_nNextBufferIdx;
        size_t m_nNextExpectedReqIdx;
        size_t m_nNextPrecacheIdx;
        size_t m_nReqIdx,m_nLastReqIdx;
        cv::Mat m_oReqPacket,m_oLastReqPacket;
    private:
        DataPrecacher& operator=(const DataPrecacher&) = delete;
        DataPrecacher(const DataPrecacher&) = delete;
    };

    template<bool bGroup>
    struct IDataLoader_ : public virtual IDataHandler {};

    template<>
    struct IDataLoader_<TNoGroup> : public virtual IDataHandler { // generalized producer (exposes common interface for all dataset types)
        IDataLoader_();
        virtual void startPrecaching(bool bPrecacheGT, size_t nSuggestedBufferSize=SIZE_MAX) override;
        virtual void stopPrecaching() override;
    protected:
        DataPrecacher m_oInputPrecacher;
        DataPrecacher m_oGTPrecacher;
        virtual cv::Mat _getInputPacket_impl(size_t nIdx) = 0;
        virtual cv::Mat _getGTPacket_impl(size_t nIdx) = 0;
    private:
        cv::Mat m_oLatestInputPacket, m_oLatestGTPacket;
        const cv::Mat& _getInputPacket_redirect(size_t nIdx);
        const cv::Mat& _getGTPacket_redirect(size_t nIdx);
    };

    template<eDatasetTypeList eDatasetType>
    struct IDataReader_;

    template<>
    struct IDataReader_<eDatasetType_VideoSegm> : public virtual IDataHandler { // contains group-impl only
        virtual size_t getFrameCount() const {return getTotPackets();}
        virtual const cv::Mat& getInputFrame(size_t nFrameIdx) {return dynamic_cast<IDataReader_<eDatasetType_VideoSegm>&>(*getBatch(nFrameIdx)).getInputFrame(nFrameIdx);}
        virtual const cv::Mat& getGTFrame(size_t nFrameIdx) {return dynamic_cast<IDataReader_<eDatasetType_VideoSegm>&>(*getBatch(nFrameIdx)).getGTFrame(nFrameIdx);}
    };

    template<eDatasetTypeList eDatasetType, bool bGroup>
    struct IDataProducer_ :
            public IDataLoader_<bGroup>,
            public IDataReader_<eDatasetType> {};

    template<> // all method impl can go in CPP as template possibilities are tightly defined
    struct IDataProducer_<eDatasetType_VideoSegm,TNoGroup> :
            public IDataLoader_<TNoGroup>,
            public IDataReader_<eDatasetType_VideoSegm> {
        IDataProducer_() : m_nFrameCount(0),m_nNextExpectedVideoReaderFrameIdx(size_t(-1)) {}

        virtual double getExpectedLoad() const override {return m_oROI.empty()?0.0:(double)cv::countNonZero(m_oROI)*m_nFrameCount*(int(!isGrayscale())+1);}
        virtual size_t getTotPackets() const override {return m_nFrameCount;}
        virtual void startPrecaching(bool bUsingGT, size_t /*nUnused*/=0) override {
            return IDataLoader_<TNoGroup>::startPrecaching(bUsingGT,m_oSize.area()*(m_nFrameCount+1)*(isGrayscale()?1:getDatasetInfo().is4ByteAligned()?4:3));
        }
        virtual cv::Size getFrameSize() const {return m_oSize;}
        virtual const cv::Mat& getROI() const {return m_oROI;}
        virtual const cv::Mat& getInputFrame(size_t nFrameIdx) override final {return m_oInputPrecacher.getPacket(nFrameIdx);}
        virtual const cv::Mat& getGTFrame(size_t nFrameIdx) override final {return m_oGTPrecacher.getPacket(nFrameIdx);}

        virtual void parseDataset() override {
            cv::Mat oTempImg;
            m_voVideoReader.open(getPath());
            if(!m_voVideoReader.isOpened()) {
                PlatformUtils::GetFilesFromDir(getPath(),m_vsInputFramePaths);
                if(!m_vsInputFramePaths.empty()) {
                    oTempImg = cv::imread(m_vsInputFramePaths[0]);
                    m_nFrameCount = m_vsInputFramePaths.size();
                }
            }
            else {
                m_voVideoReader.set(cv::CAP_PROP_POS_FRAMES,0);
                m_voVideoReader >> oTempImg;
                m_voVideoReader.set(cv::CAP_PROP_POS_FRAMES,0);
                m_nFrameCount = (size_t)m_voVideoReader.get(cv::CAP_PROP_FRAME_COUNT);
            }
            if(oTempImg.empty())
                throw std::runtime_error(cv::format("Sequence '%s': video could not be opened via VideoReader or imread (you might need to implement your own DataProducer_ interface)",getName().c_str()));
            m_oOrigSize = oTempImg.size();
            const double dScale = getDatasetInfo().getScaleFactor();
            if(dScale!=1.0)
                cv::resize(oTempImg,oTempImg,cv::Size(),dScale,dScale,cv::INTER_NEAREST);
            m_oROI = cv::Mat(oTempImg.size(),CV_8UC1,cv::Scalar_<uchar>(255));
            m_oSize = oTempImg.size();
            m_nNextExpectedVideoReaderFrameIdx = 0;
            CV_Assert(m_nFrameCount>0);
        }

    protected:

        virtual cv::Mat _getInputPacket_impl(size_t nIdx) override {
            cv::Mat oFrame;
            if(!m_voVideoReader.isOpened())
                oFrame = cv::imread(m_vsInputFramePaths[nIdx],isGrayscale()?cv::IMREAD_GRAYSCALE:cv::IMREAD_COLOR);
            else {
                if(m_nNextExpectedVideoReaderFrameIdx!=nIdx) {
                    m_voVideoReader.set(cv::CAP_PROP_POS_FRAMES,(double)nIdx);
                    m_nNextExpectedVideoReaderFrameIdx = nIdx+1;
                }
                else
                    ++m_nNextExpectedVideoReaderFrameIdx;
                m_voVideoReader >> oFrame;
                if(isGrayscale() && oFrame.channels()>1)
                    cv::cvtColor(oFrame,oFrame,cv::COLOR_BGR2GRAY);
            }
            if(getDatasetInfo().is4ByteAligned() && oFrame.channels()==3)
                cv::cvtColor(oFrame,oFrame,cv::COLOR_BGR2BGRA);
            if(oFrame.size()!=m_oSize)
                cv::resize(oFrame,oFrame,m_oSize,0,0,cv::INTER_NEAREST);
            return oFrame;
        }

        virtual cv::Mat _getGTPacket_impl(size_t) override {
            return cv::Mat(m_oSize,CV_8UC1,cv::Scalar_<uchar>(DATASETUTILS_VIDEOSEGM_OUTOFSCOPE_VAL));
        }

        size_t m_nFrameCount;
        std::vector<std::string> m_vsInputFramePaths;
        std::vector<std::string> m_vsGTFramePaths;
        cv::VideoCapture m_voVideoReader;
        size_t m_nNextExpectedVideoReaderFrameIdx;
        cv::Mat m_oROI;
        cv::Size m_oOrigSize,m_oSize;
        std::unordered_map<size_t,size_t> m_mTestGTIndexes;
    };

    template<bool bGroup>
    struct IDataCounter_;

    template<>
    struct IDataCounter_<TNoGroup> : public virtual IDataHandler { // generalized consumer (exposes common interface for all dataset types)
        IDataCounter_() : m_nProcessedPackets(0) {}
    protected:
        void processPacket() {++m_nProcessedPackets;}
        void setProcessedPacketsPromise() {m_nProcessedPacketsPromise.set_value(m_nProcessedPackets);}
        virtual size_t getProcessedPacketsCountPromise() override {return m_nProcessedPacketsPromise.get_future().get();}
        virtual size_t getProcessedPacketsCount() const override {return m_nProcessedPackets;}
    private:
        size_t m_nProcessedPackets;
        std::promise<size_t> m_nProcessedPacketsPromise;
    };

    template<>
    struct IDataCounter_<TGroup> : public virtual IDataHandler { // generalized consumer (exposes common interface for all dataset types)
        virtual size_t getProcessedPacketsCountPromise() override final {return CxxUtils::accumulateMembers<size_t,IDataHandlerPtr>(getBatches(),[](const IDataHandlerPtr& p){return p->getProcessedPacketsCountPromise();});}
        virtual size_t getProcessedPacketsCount() const override final {return CxxUtils::accumulateMembers<size_t,IDataHandlerPtr>(getBatches(),[](const IDataHandlerPtr& p){return p->getProcessedPacketsCount();});}
    };


    template<eDatasetTypeList eDatasetType>
    struct IDataRecorder_;

    template<>
    struct IDataRecorder_<eDatasetType_VideoSegm> : public virtual IDataHandler { // contains group-impl only
        virtual cv::Mat readResult(size_t nIdx) const {return dynamic_cast<IDataRecorder_<eDatasetType_VideoSegm>&>(*getBatch(nIdx)).readResult(nIdx);}
        virtual void pushResult(const cv::Mat& oSegm, size_t nIdx) {dynamic_cast<IDataRecorder_<eDatasetType_VideoSegm>&>(*getBatch(nIdx)).pushResult(oSegm,nIdx);}
        virtual void writeResult(const cv::Mat& oSegm, size_t nIdx) const {dynamic_cast<IDataRecorder_<eDatasetType_VideoSegm>&>(*getBatch(nIdx)).writeResult(oSegm,nIdx);}
    };

    template<eDatasetTypeList eDatasetType, bool bGroup>
    struct IDataConsumer_ :
            public IDataCounter_<bGroup>,
            public IDataRecorder_<eDatasetType> {};

    template<>
    struct IDataConsumer_<eDatasetType_VideoSegm,TNoGroup> :
            public IDataCounter_<TNoGroup>,
            public IDataRecorder_<eDatasetType_VideoSegm> {

        virtual cv::Mat readResult(size_t nIdx) const override {
            CV_Assert(!getDatasetInfo().getResultsNameSuffix().empty());
            std::array<char,10> acBuffer;
            snprintf(acBuffer.data(),acBuffer.size(),"%06zu",nIdx);
            std::stringstream sResultFilePath;
            sResultFilePath << getResultsPath() << getDatasetInfo().getResultsNamePrefix() << acBuffer.data() << getDatasetInfo().getResultsNameSuffix();
            return cv::imread(sResultFilePath.str(),isGrayscale()?cv::IMREAD_GRAYSCALE:cv::IMREAD_COLOR);
        }

        void pushResult(const cv::Mat& oSegm, size_t nIdx) override {
            processPacket();
            _pushResult(oSegm,nIdx);
            if(getDatasetInfo().isSavingResults())
                writeResult(oSegm,nIdx);
        }

        virtual void writeResult(const cv::Mat& oSegm, size_t nIdx) const override {
            CV_Assert(!getDatasetInfo().getResultsNameSuffix().empty());
            std::array<char,10> acBuffer;
            snprintf(acBuffer.data(),acBuffer.size(),"%06zu",nIdx);
            std::stringstream sResultFilePath;
            sResultFilePath << getResultsPath() << getDatasetInfo().getResultsNamePrefix() << acBuffer.data() << getDatasetInfo().getResultsNameSuffix();
            const std::vector<int> vnComprParams = {cv::IMWRITE_PNG_COMPRESSION,9};
            cv::imwrite(sResultFilePath.str(),oSegm,vnComprParams);
        }

    protected:
        virtual void _pushResult(const cv::Mat& /*oSegm*/, size_t /*nIdx*/) {} // should be overriden in child classes for each dataset
    };

#if 0
    namespace Video {

        namespace Segm {

            enum eDatasetList {
                eDataset_CDnet2012,
                eDataset_CDnet2014,
                eDataset_Wallflower,
                eDataset_PETS2001_D3TC1,
                // ...
                eDataset_Custom
            };

            struct DatasetInfo : public DatasetInfoBase {
                DatasetInfo();
                DatasetInfo(const std::string& sDatasetName, const std::string& sDatasetRootPath, const std::string& sResultsRootPath,
                            const std::string& sResultNamePrefix, const std::string& sResultNameSuffix, const std::vector<std::string>& vsWorkBatchPaths,
                            const std::vector<std::string>& vsSkippedNameTokens, const std::vector<std::string>& vsGrayscaleNameTokens,
                            bool bForce4ByteDataAlign, double dScaleFactor, eDatasetList eDatasetID, size_t nResultIdxOffset);
                virtual void WriteEvalResults(const std::vector<std::shared_ptr<WorkBatchGroup>>& vpGroups) const;
                virtual eDatasetTypeList GetType() const {return eDatasetType_Video_Segm;}
                eDatasetList m_eDatasetID;
                size_t m_nResultIdxOffset;
            };

            std::shared_ptr<DatasetInfo> GetDatasetInfo(eDatasetList eDatasetID, const std::string& sDatasetRootDirPath, const std::string& sResultsDirName, bool bForce4ByteDataAlign);

            class Sequence : public WorkBatch {
            public:
                Sequence(const std::string& sSeqName, const DatasetInfo& oDataset, const std::string& sRelativePath=std::string("./"));
                virtual size_t GetTotalImageCount() const {return m_nTotFrameCount;}
                virtual double GetExpectedLoad() const {return m_dExpectedLoad;}
                virtual void WriteResult(size_t nIdx, const cv::Mat& oResult);
                virtual bool StartPrecaching(bool bUsingGT, size_t nUnused=0);
                cv::Size GetImageSize() const {return m_oSize;}
                const cv::Mat& GetROI() const {return m_oROI;}
                const eDatasetList m_eDatasetID;
                const size_t m_nResultIdxOffset;
            protected:
                virtual cv::Mat GetInputFromIndex_external(size_t nFrameIdx);
                virtual cv::Mat GetGTFromIndex_external(size_t nFrameIdx);
            private:
                double m_dExpectedLoad;
                size_t m_nTotFrameCount;
                std::vector<std::string> m_vsInputFramePaths;
                std::vector<std::string> m_vsGTFramePaths;
                cv::VideoCapture m_voVideoReader;
                size_t m_nNextExpectedVideoReaderFrameIdx;
                cv::Mat m_oROI;
                cv::Size m_oOrigSize,m_oSize;
                double m_dScaleFactor;
                std::unordered_map<size_t,size_t> m_mTestGTIndexes;
                Sequence& operator=(const Sequence&) = delete;
                Sequence(const Sequence&) = delete;
            };

        } //namespace Segm

        namespace Registr {

            enum eDatasetList {
                eDataset_LITIV2012b,
                // ...
                eDataset_Custom
            };

            struct DatasetInfo : public DatasetInfoBase {
                DatasetInfo();
                DatasetInfo(const std::string& sDatasetName, const std::string& sDatasetRootPath, const std::string& sResultsRootPath,
                            const std::string& sResultNamePrefix, const std::string& sResultNameSuffix, const std::vector<std::string>& vsWorkBatchPaths,
                            const std::vector<std::string>& vsSkippedNameTokens, const std::vector<std::string>& vsGrayscaleNameTokens,
                            bool bForce4ByteDataAlign, double dScaleFactor, eDatasetList eDatasetID, size_t nResultIdxOffset);
                virtual void WriteEvalResults(const std::vector<std::shared_ptr<WorkBatchGroup>>& vpGroups) const;
                virtual eDatasetTypeList GetType() const {return eDatasetType_Video_Registr;}
                eDatasetList m_eDatasetID;
                size_t m_nResultIdxOffset;
            };

            std::shared_ptr<DatasetInfo> GetDatasetInfo(eDatasetList eDatasetID, const std::string& sDatasetRootDirPath, const std::string& sResultsDirName, bool bForce4ByteDataAlign);

            class Sequence : public WorkBatch {
            public:
                Sequence(const std::string& sSeqName, const DatasetInfo& oDataset, const std::string& sRelativePath=std::string("./"));
                virtual size_t GetTotalImageCount() const {return m_nTotFrameCount;}
                virtual double GetExpectedLoad() const {return m_dExpectedLoad;}
                virtual void WriteResult(size_t nIdx, const cv::Mat& oResult);
                virtual bool StartPrecaching(bool bUsingGT, size_t nUnused=0);
                cv::Size GetImageSize() const {return m_oSize;}
                const cv::Mat& GetROI() const {return m_oROI;}
                const eDatasetList m_eDatasetID;
                const size_t m_nResultIdxOffset;
            protected:
                virtual cv::Mat GetInputFromIndex_external(size_t nFrameIdx);
                virtual cv::Mat GetGTFromIndex_external(size_t nFrameIdx);
            private:
                double m_dExpectedLoad;
                size_t m_nTotFrameCount;
                std::vector<std::string> m_vsInputFramePaths;
                std::vector<std::string> m_vsGTFramePaths;
                cv::VideoCapture m_voVideoReader;
                size_t m_nNextExpectedVideoReaderFrameIdx;
                cv::Mat m_oROI;
                cv::Size m_oOrigSize,m_oSize;
                double m_dScaleFactor;
                std::unordered_map<size_t,size_t> m_mTestGTIndexes;
                Sequence& operator=(const Sequence&) = delete;
                Sequence(const Sequence&) = delete;
            };
        } //namespace Registr

    } //namespace Video

    namespace Image {

        namespace Segm {

            enum eDatasetList {
                eDataset_BSDS500_segm_train,
                eDataset_BSDS500_segm_train_valid,
                eDataset_BSDS500_segm_train_valid_test,
                eDataset_BSDS500_edge_train,
                eDataset_BSDS500_edge_train_valid,
                eDataset_BSDS500_edge_train_valid_test,
                // ...
                eDataset_Custom
            };

            struct DatasetInfo : public DatasetInfoBase {
                DatasetInfo();
                DatasetInfo(const std::string& sDatasetName, const std::string& sDatasetRootPath, const std::string& sResultsRootPath,
                            const std::string& sResultNamePrefix, const std::string& sResultNameSuffix, const std::vector<std::string>& vsWorkBatchPaths,
                            const std::vector<std::string>& vsSkippedNameTokens, const std::vector<std::string>& vsGrayscaleNameTokens,
                            bool bForce4ByteDataAlign, double dScaleFactor, eDatasetList eDatasetID);
                virtual void WriteEvalResults(const std::vector<std::shared_ptr<WorkBatchGroup>>& vpGroups) const;
                virtual eDatasetTypeList GetType() const {return eDatasetType_Image_Segm;}
                eDatasetList m_eDatasetID;
            };

            std::shared_ptr<DatasetInfo> GetDatasetInfo(eDatasetList eDatasetID, const std::string& sDatasetRootPath, const std::string& sResultsDirName, bool bForce4ByteDataAlign);

            class Set : public WorkBatch {
            public:
                Set(const std::string& sSetName, const DatasetInfo& oDataset, const std::string& sRelativePath=std::string("./"));
                virtual size_t GetTotalImageCount() const {return m_nTotImageCount;}
                virtual double GetExpectedLoad() const {return m_dExpectedLoad;}
                virtual cv::Mat ReadResult(size_t nIdx);
                virtual void WriteResult(size_t nIdx, const cv::Mat& oResult);
                virtual bool StartPrecaching(bool bUsingGT, size_t nUnused=0);
                bool IsConstantImageSize() const {return m_bIsConstantSize;}
                cv::Size GetMaxImageSize() const {return m_oMaxSize;}
                const eDatasetList m_eDatasetID;
            protected:
                virtual cv::Mat GetInputFromIndex_external(size_t nImageIdx);
                virtual cv::Mat GetGTFromIndex_external(size_t nImageIdx);
            private:
                double m_dExpectedLoad;
                size_t m_nTotImageCount;
                std::vector<std::string> m_vsInputImagePaths;
                std::vector<std::string> m_vsGTImagePaths;
                std::vector<std::string> m_vsOrigImageNames;
                std::vector<cv::Size> m_voOrigImageSizes;
                cv::Size m_oMaxSize;
                bool m_bIsConstantSize;
                Set& operator=(const Set&) = delete;
                Set(const Set&) = delete;
            };

        } //namespace Segm

    } //namespace Image

#endif

} //namespace litiv
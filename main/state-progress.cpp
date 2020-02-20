#include "state-progress.h"
#include "state-board.h"
#include "player.h"
#include "timer.h"
#include "game-def.h"
#include "animate.h"

namespace stateProgress {
    byte _faceBuffer[FACE_COUNT];
    bool _drawMoves;
    
    #define CONFLICT_RATE 4
    #define PHASE_DURATION 2000
    
    void handleBloomDone(){
        stateCommon::handleStateChange(GAME_DEF_STATE_BOARD);

    }
    static void handleBloom() {
        if(_drawMoves) {
            return;
        }
        FOREACH_FACE(f) {
            if(_faceBuffer[f] < PLAYER_LIMIT) {
                stateBoard::applyOwner(f, _faceBuffer[f]);
                animate::pulseFace(f, player::getColor(_faceBuffer[f]), 4);
            }        
        }
    }

    void popBloomFaceBuffer(byte f) {
        byte priorOwner = stateBoard::getOwnershipe( (FACE_COUNT + f + 1) % FACE_COUNT);
        byte nextOwner = stateBoard::getOwnershipe( (FACE_COUNT + f - 1) % FACE_COUNT);
        byte offOwner = stateBoard::getOffOwnershipe(f);
        //Two match, they win!
        if(priorOwner < PLAYER_LIMIT && (priorOwner == nextOwner || priorOwner == offOwner)) {
            _faceBuffer[f] = priorOwner;
            return;
        }
        if(nextOwner < PLAYER_LIMIT && nextOwner == offOwner) {
            _faceBuffer[f] = nextOwner;
            return;
        }

        //one and no others, they win!
        if(priorOwner < PLAYER_LIMIT    && nextOwner >= PLAYER_LIMIT    && offOwner >= PLAYER_LIMIT) {
            _faceBuffer[f] = priorOwner;
            return;
        }
        if(priorOwner >= PLAYER_LIMIT   && nextOwner < PLAYER_LIMIT     && offOwner >= PLAYER_LIMIT) {
            _faceBuffer[f] = nextOwner;
            return;
        }
        if(priorOwner >= PLAYER_LIMIT   && nextOwner >= PLAYER_LIMIT    && offOwner < PLAYER_LIMIT) {
            _faceBuffer[f] = offOwner;
            return;
        }
    }
    static void popBloomFaces(){
        FOREACH_FACE(f){
            _faceBuffer[f] = PLAYER_LIMIT;
            if(stateBoard::getOwnershipe(f) >= PLAYER_LIMIT) {
                popBloomFaceBuffer(f);
            }
        }
    }

    void handleMovesDone(){
        _drawMoves = false;
        popBloomFaces();
        timer::mark(PHASE_DURATION, handleBloomDone);
    }
    void handleDelayedOwnerUpdate(){
        stateBoard::updateOffOwners();
        timer::mark(PHASE_DURATION/2, handleMovesDone);
    }

    static void handleMoves() {
        if(!_drawMoves) {
            return;
        }
        FOREACH_FACE(f) {
            byte count = stateBoard::getRequestsForFace(f, &(_faceBuffer[0]));
            if(count == 0) {
                continue;
            }
            if(count == 1) {
                animate::pulseFace(f, player::getColor(_faceBuffer[0]), 4);
                continue;
            }
            byte period = PHASE_DURATION / (count*2);
            byte current = (timer::runningFor() / period) % count;
            setColorOnFace(player::getColor(_faceBuffer[current]), f);
        }
        return;
    }

    void loop(const bool isEnter, const stateCommon::LoopData& data) {
        if(isEnter) {
             _drawMoves = true;
            timer::mark(PHASE_DURATION/2, handleDelayedOwnerUpdate);
        }
        stateBoard::drawOwners();
        handleMoves();
        handleBloom();
        
    }
}
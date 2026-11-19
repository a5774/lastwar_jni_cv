package com.example.supercell;

import android.app.Dialog;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AlertDialog;

import android.view.Window;


public class CheckDialogFragment extends DialogFragment {

    public interface DialogCallback {
        default void onConfirm() {
        }
        default void onCancel() {
        }
    }

    private static final String TITLE_MESSAGE = "title_message";
    private static final String MAIN_MESSAGE = "main_message";
    private static final String POS_MESSAGE = "pos_message";
    private static final String NEG_MESSAGE = "neg_message";

    private DialogCallback callback;

    public static CheckDialogFragment newInstance(String check, String message, String positive, String negative, DialogCallback callback) {
        CheckDialogFragment fragment = new CheckDialogFragment();
        Bundle args = new Bundle();
        args.putString(TITLE_MESSAGE, check);
        args.putString(MAIN_MESSAGE, message);
        args.putString(POS_MESSAGE, positive);
        args.putString(NEG_MESSAGE, negative);
        fragment.setArguments(args);
        fragment.setCallback(callback);
        return fragment;
    }

    public void setCallback(DialogCallback callback) {
        this.callback = callback;
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        Bundle bundle = null;
        if (this.getArguments() != null) {
            bundle = this.getArguments();
        }
        AlertDialog dialog = new AlertDialog.Builder(requireContext())
                .setTitle(MiscUtils.getStringWithDefault(bundle, TITLE_MESSAGE, "提示"))
                .setMessage(MiscUtils.getStringWithDefault(bundle, MAIN_MESSAGE, "内部错误"))
//                dialog_ 使用DialogInterface 接收
                .setPositiveButton(MiscUtils.getStringWithDefault(bundle, POS_MESSAGE, "确定"), (dialog_, which) -> {
                    if (callback != null) {
                        callback.onConfirm();
                    }
//                    默认行为,在setOnShowListener中拦截
                    dismiss();
                })
                .setNegativeButton(MiscUtils.getStringWithDefault(bundle, NEG_MESSAGE, "取消"), (dialog_, which) -> {
                    if (callback != null) {
                        callback.onCancel();
                    }
                }).setCancelable(false)
                .create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.setCancelable(false);
        dialog.setOnShowListener(d -> {
            Window window = dialog.getWindow();
            if (window != null) {
                window.getDecorView().setSystemUiVisibility(MiscUtils.UIOptions);
            }
        });
        return dialog;
    }
}
